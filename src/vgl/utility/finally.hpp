#pragma once

namespace vgl::cpp {
    template <typename Function>
    class final_op {
    public:
        explicit final_op(Function f) noexcept
            : _function(std::move(f)), _invoke(true) {}

        final_op(final_op&& other) noexcept
            : _function(std::move(other._function)),
            _invoke(other._invoke) {
            other._invoke = false;
        }

        final_op(const final_op&) = delete;
        final_op& operator=(const final_op&) = delete;

        ~final_op() noexcept {
            if (_invoke) {
                _function();
            }
        }

    private:
        Function _function;
        bool _invoke;
    };
}
