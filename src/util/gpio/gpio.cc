#include "util/gpio/gpio.hpp"

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace gsync {

const std::string Gpio::kGpioPathPrefix("/sys/class/gpio/");

static bool FileExists(const std::string& filename) {
    struct stat buffer;
    return (0 == stat(filename.c_str(), &buffer));
}

static bool Write(const std::string path, const std::string filename,
                  const std::string value) {
    std::ofstream fs((path + filename).c_str());
    if (!fs) {
        perror("write failed to open file ");
        return false;
    }
    fs << value;
    return true;
}

static bool Write(const std::string& path, const std::string& filename,
                  int value) {
    std::stringstream ss;
    ss << value;
    return Write(path, filename, ss.str());
}

static std::string Read(const std::string& path, const std::string& filename) {
    std::ifstream fs((path + filename).c_str());
    if (!fs) {
        perror("read failed to open file");
        return "";
    }
    std::string input("");
    std::getline(fs, input);
    return input;
}

bool Gpio::ExportGpio() const {
    if (!FileExists(kGpioPathPrefix + "gpio" + std::to_string(number_))) {
        return Write(kGpioPathPrefix, "export", number_);
    }
    return true;
}

bool Gpio::UnexportGpio() const {
    if (FileExists(kGpioPathPrefix + "gpio" + std::to_string(number_))) {
        return Write(kGpioPathPrefix, "unexport", number_);
    }
    return true;
}

Gpio::Gpio(int number)
    : number_(number),
      name_("gpio" + std::to_string(number_)),
      path_(kGpioPathPrefix + name_ + "/") {
    if ((number <= 0) || !ExportGpio()) {
        throw std::runtime_error("invalid pin number");
    }
    static const int kSysFsSetupDelayUsec = 250000;
    usleep(kSysFsSetupDelayUsec);
}

void Gpio::Dir(Direction dir) const {
    switch (dir) {
        case Direction::kInput:
            Write(path_, "direction", "in");
            break;
        case Direction::kOutput:
            Write(path_, "direction", "out");
            break;
    }
}

Gpio::Direction Gpio::Dir() const {
    std::string input = Read(path_, "direction");
    return (input == "in") ? Direction::kInput : Direction::kOutput;
}

void Gpio::Val(Value value) const {
    switch (value) {
        case kHigh:
            Write(path_, "value", "1");
            break;
        case kLow:
            Write(path_, "value", "0");
            break;
    }
}

Gpio::Value Gpio::Val() const {
    std::string input = Read(path_, "value");
    return (input == "0") ? Value::kLow : Value::kHigh;
}

void Gpio::ToggleOutput() const {
    Dir(Direction::kOutput);
    switch (Val()) {
        case Value::kLow:
            Val(Value::kHigh);
            break;
        case Value::kHigh:
            Val(Value::kLow);
            break;
    }
}

void Gpio::SetActiveLow(bool is_low) const {
    if (is_low) {
        Write(path_, "active_low", "1");
    } else {
        Write(path_, "active_low", "0");
    }
}

void Gpio::EdgeType(Edge edge) const {
    switch (edge) {
        case Edge::kNone:
            Write(path_, "edge", "none");
            break;
        case Edge::kRising:
            Write(path_, "edge", "rising");
            break;
        case Edge::kFalling:
            Write(path_, "edge", "falling");
            break;
        case Edge::kBoth:
            Write(path_, "edge", "both");
            break;
    }
}

Gpio::Edge Gpio::EdgeType() const {
    std::string input = Read(path_, "edge");
    if (input == "rising") {
        return Edge::kRising;
    } else if (input == "falling") {
        return Edge::kFalling;
    } else if (input == "both") {
        return Edge::kBoth;
    } else {
        return Edge::kNone;
    }
}

bool Gpio::WaitForEdge() {
    /* We have to set the pin to be an input pin order to poll it. */
    Dir(Direction::kInput);

    int epollfd = epoll_create(1);
    if (-1 == epollfd) {
        return false;
    }

    int fd = open((path_ + "value").c_str(), O_RDONLY | O_NONBLOCK);
    if (-1 == fd) {
        return false;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLPRI;
    ev.data.fd = fd;

    /* register the file descriptor on the epoll instance. */
    if (-1 == epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev)) {
        return false;
    }

    int events = 0;
    bool success = true;
    while (events <= 1) { /* we ignore the first event */
        int rc = epoll_wait(epollfd, &ev, 1, -1);
        if (-1 == rc) {
            success = false;
            break;
        }
        events++;
    }
    close(fd);

    return success;
}

}  // namespace gsync
