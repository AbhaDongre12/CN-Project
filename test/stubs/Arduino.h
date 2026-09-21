#pragma once

#include <stdint.h>
#include <iostream>

using byte = uint8_t;

class SerialStub
{
public:
    template <typename T>
    void print(const T &value)
    {
        std::cout << value;
    }

    void print(uint8_t value)
    {
        std::cout << static_cast<unsigned int>(value);
    }

    template <typename T>
    void println(const T &value)
    {
        std::cout << value << std::endl;
    }

    void println(uint8_t value)
    {
        std::cout << static_cast<unsigned int>(value) << std::endl;
    }

    void println()
    {
        std::cout << std::endl;
    }
};

extern SerialStub Serial;

inline unsigned long millis()
{
    return 0;
}