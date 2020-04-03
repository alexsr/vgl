#pragma once

namespace vgl::cpp {
    template <typename Function>
    class final_op {
    public:
        explicit final_op(Function f) noexcept
            : function_(std::move(f)), invoke_(true) {}

        final_op(final_op&& other) noexcept
            : function_(std::move(other.function_)),
            invoke_(other.invoke_) {
            other.invoke_ = false;
        }

        final_op(const final_op&) = delete;
        final_op& operator=(const final_op&) = delete;

        ~final_op() noexcept {
            if (invoke_) {
                function_();
            }
        }

    private:
        Function function_;
        bool invoke_;
    };
}
