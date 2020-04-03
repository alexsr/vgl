#pragma once

#include <chrono>

namespace vgl {
    struct frame_timer {
        virtual void new_frame() {
            delta_time = current - previous;
        }
        virtual double dt() const {
            return delta_time.count();
        }
        virtual double s_per_frame() const {
            return dt();
        }
        virtual double ms_per_frame() const {
            return s_per_frame() * 1000.0;
        }
        virtual double fps() const {
            return 1.0 / s_per_frame();
        }
    protected:
        std::chrono::high_resolution_clock::time_point current =
            std::chrono::high_resolution_clock::now();
        std::chrono::high_resolution_clock::time_point previous =
            std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> delta_time{ 0 };
    };

    struct accumulated_frame_timer : public frame_timer {
        accumulated_frame_timer(double interval = 1.0) : interval(interval) {}
        void new_frame() override {
            delta_time = current - previous;
            previous = current;
            current = std::chrono::high_resolution_clock::now();
            accumulator += dt();
            frames++;
            if (accumulator > interval) {
                s_p_f = accumulator / static_cast<double>(frames);
                frames = 0;
                accumulator = 0.0;
            }
        }
        double s_per_frame() const override {
            return s_p_f;
        }
    private:
        double accumulator = 0.0;
        int frames = 0;
        double s_p_f = 0.0;
        double interval = 1.0;
    };
}
