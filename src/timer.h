#pragma once

#include <chrono>
#include <iostream>
#include <string>

class Timer {
public:
    Timer() { reset(); }

    void reset() { m_start = std::chrono::high_resolution_clock::now(); }

    float elapsed()
    {
        return static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                      std::chrono::high_resolution_clock::now() - m_start)
                                      .count()) *
               0.001f * 0.001f * 0.001f;
    }

    float elapsed_millis() { return elapsed() * 1000.0f; }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

// class ScopedTimer
// {
// public:
//     explicit ScopedTimer(const std::string &name) : m_name(name) {}
//     ~ScopedTimer() // NOLINT(cppcoreguidelines-special-member-functions): not using heap allocated memory here
//     {
//         const float time = m_timer.elapsed_millis();
//          LOG
//     }

// private:
//     std::string m_name;
//     Timer m_timer;
// };