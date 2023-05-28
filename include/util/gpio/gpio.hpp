#ifndef GPIO_H_
#define GPIO_H_

#include <gpiod.hpp>
#include <string>

namespace gsync {

/**
 * GPIO control utility.
 *
 * A wrapper around libgpiod's C++ bindings. Note, Gpio propagates the std
 * exceptions that are thrown by libgpiod!
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
     * Construct a GPIO controller.
     *
     * @param[in] dev GPIO device name (e.g., /dev/gpiochip0).
     * @param[in] line GPIO line offset.
     * @param[in] name Optional GPIO pin name.
     */
    Gpio(const std::string& dev, int offset,
         const std::string& name = "unnamed");

    Gpio() = delete;

    ~Gpio() = default;
    Gpio(const Gpio&) = default;
    Gpio& operator=(const Gpio&) = default;
    Gpio(Gpio&&) = default;
    Gpio& operator=(Gpio&&) = default;

    /** Return the chip label. */
    std::string ChipLabel() const { return chip_.label(); }

    /** Return the chip name. */
    std::string ChipName() const { return chip_.name(); }

    /** Return the line name. */
    std::string LineName() const { return line_.name(); }

    /** Return the line offset. */
    unsigned int LineOffset() const { return line_.offset(); }

    /** Set the GPIO in/out direction. */
    void Dir(Direction direction);

    /** Return the current in/out direction of the GPIO. */
    Direction Dir() const { return dir_; }

    /** Set the GPIO to a low/high value. */
    void Val(Value value) const;

    /** Return the current low/high value of the GPIO. */
    Value Val() const;

    /** Set the GPIO edge type. */
    void EdgeType(Gpio::Edge edge);

    /** Return the current GPIO edge setting. */
    Gpio::Edge EdgeType() const { return edge_; }

    /**
     * Set or clear the \a active_low setting of the GPIO
     *
     * @param[in] is_low If set to true, the GPIO active_low property will be
     * set, otherwise, the \a active_low property will be cleared.
     */
    void SetActiveLow(bool is_low = true) const;

    /** Set the GPIO to active high. */
    void SetActiveHigh() const { SetActiveLow(false); }

    /** Toggle the GPIO output value. */
    void ToggleOutput() const;

    /** Block indefinitely until an edge triggered event is detected. */
    void WaitForEdge();

   private:
    gpiod::chip chip_;
    gpiod::line line_;
    std::string name_;
    Direction dir_;
    Edge edge_;
};

}  // namespace gsync

#endif
