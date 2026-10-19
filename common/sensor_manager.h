#pragma once

#include <Arduino.h>
#include "sensor_interface.h"
#include "sensor_config.h"

// Small fixed-capacity manager: add/remove sensors by editing the config
// array, not by duplicating sensor-reading code in main.cpp.
class SensorManager
{
public:
    bool begin(const SensorConfig *config, size_t count)
    {
        if (config == nullptr || count == 0 || count > MAX_CONFIGURED_SENSORS)
        {
            count_ = 0;
            return false;
        }
        count_ = count;
        for (size_t i = 0; i < count_; ++i)
        {
            sensors_[i].configure(config[i].trig_pin, config[i].echo_pin,
                                  config[i].zone, config[i].detection_max_cm);
            sensors_[i].begin();
        }
        nextIndex_ = 0;
        return true;
    }

    size_t count() const { return count_; }

    // Call sequentially with a gap between reads to reduce HC-SR04 crosstalk.
    bool read(size_t index, SensorReading *reading)
    {
        if (index >= count_ || reading == nullptr)
            return false;
        return sensors_[index].read(reading);
    }

    size_t nextIndex()
    {
        if (count_ == 0)
            return 0;
        const size_t current = nextIndex_;
        nextIndex_ = (nextIndex_ + 1) % count_;
        return current;
    }

private:
    SensorInterface sensors_[MAX_CONFIGURED_SENSORS];
    size_t count_ = 0;
    size_t nextIndex_ = 0;
};
