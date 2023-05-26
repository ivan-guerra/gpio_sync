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
        kInput,  /**< Input pin. */
        kOutput, /**< Output pin. */
    };

    /** Pin high/low values. */
    enum Value {
        kLow = 0,  /**< Line is low. */
        kHigh = 1, /**< Line is high. */
    };

    /** Pin edge types. */
    enum class Edge {
        kNone,    /**< No edge setting. */
        kRising,  /**< Rising edge. */
        kFalling, /**< Falling edge. */
        kBoth,    /**< Both rising and falling edge. */
    };

    /**
     * Construct a GPIO pin controller for the pin with the parameter number.
     *
     * The pin number passed to the constructor is the internal GPIO number.
     * As an example, header label GPIOP1_17 translates to GPIO number (32 x 1)
     * \+ 17 = 49.
     *
     * @param[in] number Export GPIO number.
     *
     * @throws std::runtime_error
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

    /** Return the GPIO number exported to \a /sys/class/gpio/export */
    int SysFsNumber() const { return number_; }

    /** Set the GPIO in/out direction. */
    void Dir(Direction direction) const;

    /** Return the current in/out direction of the GPIO. */
    Direction Dir() const;

    /** Set the GPIO to a low/high value. */
    void Val(Value value) const;

    /** Return the current low/high value of the GPIO. */
    Value Val() const;

    /**
     * Toggle the GPIO output value.
     *
     * Toggle the output value of the GPIO. If the GPIO is configured as an
     * input pin, ToggleOutput() will do nothing.
     */
    void ToggleOutput() const;

    /**
     * Set or clear the \a active_low setting of the GPIO
     *
     * @param[in] is_low If set to true, the GPIO active_low property will be
     * set, otherwise, the \a active_low property will be cleared.
     */
    void SetActiveLow(bool is_low = true) const;

    /** Set the GPIO to active high. */
    void SetActiveHigh() const { SetActiveLow(false); }

    /* The original source of this code claims a ~20x speedup when writing GPIO
     * values using stream operations versus using a write() sys call. */

    /** Open an output stream to the GPIO's \a value file. */
    void StreamOpen() { stream_.open((path_ + "value").c_str()); }

    /** Write the parameter value to the GPIO \a value file. */
    void StreamWrite(Value value) { stream_ << value << std::flush; }

    /** Close an output stream previously opened with a call to StreamOpen(). */
    void StreamClose() { stream_.close(); }

    /** Set the GPIO edge type. */
    void EdgeType(Gpio::Edge edge) const;

    /** Return the current GPIO edge setting. */
    Gpio::Edge EdgeType() const;

    /**
     * Block indefinitely until an edge trigger event is detected.
     *
     * The GPIO must be configured as an input pin, otherwise, WaitForEdge()
     * will always return false.
     *
     * @return True if an event is detected. On error, false is returned and
     * \a errno is set.
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
