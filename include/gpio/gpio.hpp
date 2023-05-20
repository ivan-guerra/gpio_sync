#ifndef GPIO_H_
#define GPIO_H_

#include <fstream>
#include <string>

namespace gsync {

/**
 * GPIO pin control utility.
 *
 * This source code is a modified version of Derek Molloy's GPIO control code
 * from the book "Exploring BeagleBone".
 */
class Gpio {
   public:
    /** Pin input/output settings. */
    enum class Direction {
        kInput,
        kOutput,
    };

    /** Pin high/low values. */
    enum Value {
        kLow = 0,
        kHigh = 1,
    };

    /** Pin edge types. */
    enum class Edge {
        kNone,
        kRising,
        kFalling,
        kBoth,
    };

    /**
     * Construct a GPIO pin controller for the pin with the parameter number.
     *
     * The pin number passed to the constructor is the internal GPIO number.
     * As an example, header label GPIOP1_17 translates to GPIO number (32 * 1)
     * + 17 = 49.
     *
     * @param [in] number Export GPIO number.
     */
    explicit Gpio(int number);

    /**
     * GPIO pin controller destructor.
     *
     * The destructor will unexport the pin number given at construction.
     */
    ~Gpio() { UnexportGpio(); }

    /* The following constructors/assignment overloads are deleted because the
     * class uses a stream object that does not support copy/move. */
    Gpio() = delete;
    Gpio(const Gpio&) = delete;
    Gpio& operator=(const Gpio&) = delete;
    Gpio(Gpio&&) = delete;
    Gpio& operator=(Gpio&&) = delete;

    /** Return the GPIO number exported to /sys/class/gpio */
    int SysFsNumber() const { return number_; }

    /** Return true if the GPIO direction is set to the parameter direction. */
    bool Dir(Direction direction) const;

    /** Return the current in/out direction of the GPIO. */
    Direction Dir() const;

    /** Return true if the GPIO has the parameter value. */
    bool Val(Value value) const;

    /** Return the current low/high value of the GPIO. */
    Value Val() const;

    /**
     * Toggle the GPIO value
     *
     * This method has the side effect of setting the GPIO to be an output pin.
     */
    void ToggleOutput() const;

    /**
     * Set or clear the active_low setting of the GPIO
     *
     * @param is_low [in] If set to true, the GPIO active_low property will be
     * set, otherwise, the active_low property will be cleared.
     */
    bool SetActiveLow(bool is_low = true) const;

    /** Return true if the GPIO is set active high. */
    bool SetActiveHigh() const { return SetActiveLow(false); }

    /* The original source of this code claims a ~20x speedup when writing GPIO
     * values using stream operations versus using a write() sys call. */

    /** Open an output stream to the GPIO's value file. */
    void StreamOpen() { stream_.open((path_ + "value").c_str()); }

    /** Write the parameter value to the GPIO value file. */
    void StreamWrite(Value value) { stream_ << value << std::flush; }

    /** Close an output stream previously opened with a call to StreamOpen(). */
    void StreamClose() { stream_.close(); }

    /** Return true if the value in the GPIO's edge file is set to edge. */
    bool EdgeType(Gpio::Edge edge) const;

    /** Return the current edge setting of the GPIO. */
    Gpio::Edge EdgeType() const;

    /**
     * Block indefinitely until an edge trigger event is detected.
     *
     * This method has the side effect of setting the GPIO to be an input pin.
     *
     * @return True if an event is detected. On error, false is returned and
     * errno is set.
     */
    bool WaitForEdge();

   private:
    static const std::string kGpioPathPrefix;

    bool ExportGpio() const;
    bool UnexportGpio() const;

    int number_;       /**< The GPIO number of the object. */
    std::string name_; /**< The name of the GPIO (e.g., gpio50) */
    std::string path_; /**< Path to the GPIO (e.g., /sys/class/gpio/gpio50) */
    std::ofstream stream_; /**< Stream object used to write GPIO values. */
};

}  // namespace gsync

#endif
