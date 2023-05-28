#include "util/gpio/gpio.hpp"

#include <chrono>
#include <string>

namespace gsync {

Gpio::Gpio(const std::string& dev, int offset, const std::string& name)
    : chip_(dev), name_(name), dir_(Direction::kOutput), edge_(Edge::kNone) {
    line_ = chip_.get_line(offset);
}

void Gpio::Dir(Direction dir) {
    switch (dir) {
        case Direction::kInput:
            line_.request({name_, gpiod::line_request::DIRECTION_INPUT, 0});
            dir_ = Direction::kInput;
            break;
        case Direction::kOutput:
            line_.request({name_, gpiod::line_request::DIRECTION_OUTPUT, 0});
            dir_ = Direction::kOutput;
            break;
    }
}

void Gpio::Val(Value value) const {
    switch (value) {
        case kHigh:
            line_.set_value(1);
            break;
        case kLow:
            line_.set_value(0);
            break;
    }
}

Gpio::Value Gpio::Val() const {
    return (line_.get_value()) ? Value::kHigh : Value::kLow;
}

void Gpio::EdgeType(Edge edge) {
    dir_ = Direction::kInput;

    switch (edge) {
        case Edge::kRising:
            line_.request({name_, gpiod::line_request::EVENT_RISING_EDGE, 0});
            edge_ = Edge::kRising;
            break;
        case Edge::kFalling:
            line_.request({name_, gpiod::line_request::EVENT_FALLING_EDGE, 0});
            edge_ = Edge::kFalling;
            break;
        case Edge::kBoth:
            line_.request({name_, gpiod::line_request::EVENT_BOTH_EDGES, 0});
            edge_ = Edge::kBoth;
            break;
        case Edge::kNone:
            /* gpiod doesn't seem to have a 'none' edge setting. */
            edge_ = Edge::kNone;
            break;
    }
}

void Gpio::SetActiveLow(bool is_low) const {
    if (is_low) {
        line_.request({name_, gpiod::line::ACTIVE_LOW, 0});
    } else {
        line_.request({name_, gpiod::line::ACTIVE_HIGH, 0});
    }
}

void Gpio::ToggleOutput() const {
    switch (Val()) {
        case Value::kLow:
            Val(Value::kHigh);
            break;
        case Value::kHigh:
            Val(Value::kLow);
            break;
    }
}

void Gpio::WaitForEdge() {
    while (!line_.event_wait(std::chrono::seconds(1))) {
    }
    line_.event_read(); /* Consume the line event. */
}

}  // namespace gsync
