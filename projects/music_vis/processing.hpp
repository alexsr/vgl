#include <vector>
#include <cmath>
#include <complex>
#include <algorithm>
#include <execution>
#include <functional>

float hann_function(float x, float bound) {
    auto res = std::sin(glm::pi<float>() * x / (bound));
    return res * res;
}

struct Exp_moving_average {
    Exp_moving_average(float N) : _alpha(2.0f / (N + 1)), _one_minus_alpha(1.0f - _alpha) {}
    void add(float x) {
        _sum = x + _one_minus_alpha * _sum;
        _count = 1.0f + _one_minus_alpha * _count;
        ema = _sum / _count;
    }
    /*float ema() {
        return _sum / _count;
    }*/
    float ema = 0.0f;
private:
    float _alpha;
    float _one_minus_alpha;
    float _sum = 0.0f;
    float _count = 1.0f;
};


struct Exp_moving_average_buffer {
    Exp_moving_average_buffer(float N, size_t size) : _alpha(2.0f / (N + 1)), _one_minus_alpha(1.0f - _alpha) {
        _sum = std::vector<float>(size);
        ema = std::vector<float>(size);
    }

    void add(const std::vector<float>& x) {
        //if (x.size() == _sum.size()) {
        _count = 1.0f + _one_minus_alpha * _count;
        for (int i = 0; i < _sum.size(); ++i) {
            _sum.at(i) = x.at(i) + _one_minus_alpha * _sum.at(i);
            ema.at(i) = _sum.at(i) / _count;
        }
        //}
    }

    void reset() {
        std::fill(_sum.begin(), _sum.end(), 0.0f);
        _count = 1.0f;
    }
    std::vector<float> ema;

private:
    float _alpha;
    float _one_minus_alpha;
    std::vector<float> _sum;
    float _count = 1.0f;
};

// takes wavebuffer as input where output is 2 * size of wavebuffer due to complex numbers
// the wavebuffer is interleaved into output before function call
void ct_fft(const float* input, std::complex<float>* output, int n, int stride) {
    if (n == 1) {
        output[0] = std::complex<float>(input[0]);
    }
    else {
        auto h_n = n / 2;
        ct_fft(input, output, h_n, 2 * stride);
        ct_fft(input + stride, output + h_n, h_n, 2 * stride);
        for (int k = 0; k < h_n; ++k) {
            auto temp = output[k];
            /*auto X_real = std::cos(2.0f * glm::pi<float>() * k / static_cast<float>(n)) * output[k + h_n].first;
            auto X_img = std::sin(2.0f * glm::pi<float>() * k / static_cast<float>(n)) * output[k + h_n].second;*/
            auto x = std::polar(1.0f, -2.0f * glm::pi<float>()
                * static_cast<float>(k) / static_cast<float>(n)) * output[k + h_n];
            output[k] = temp + x;
            output[k + h_n] = temp - x;
        }
    }
}

// takes wavebuffer as input where output is 2 * size of wavebuffer due to complex numbers
// the wavebuffer is interleaved into output before function call
void ct_fft(const std::complex<float>* input, std::complex<float>* output, int n, int stride) {
    if (n == 1) {
        output[0] = input[0];
    }
    else {
        auto h_n = n / 2;
        ct_fft(input, output, h_n, 2 * stride);
        ct_fft(input + stride, output + h_n, h_n, 2 * stride);
        for (int k = 0; k < h_n; ++k) {
            auto temp = output[k];
            /*auto X_real = std::cos(2.0f * glm::pi<float>() * k / static_cast<float>(n)) * output[k + h_n].first;
            auto X_img = std::sin(2.0f * glm::pi<float>() * k / static_cast<float>(n)) * output[k + h_n].second;*/
            auto x = std::exp(std::complex<float>(0, -1) * 2.0f * glm::pi<float>()
                * static_cast<float>(k) / static_cast<float>(n)) * output[k + h_n];
            output[k] = temp + x;
            output[k + h_n] = temp - x;
        }
    }
}

std::vector<std::complex<float>> cooley_tukey_fft(const std::vector<float>& input) {
    std::vector<std::complex<float>> output(input.size());
    ct_fft(input.data(), output.data(), input.size(), 1);
    return output;
}

std::vector<std::complex<float>> cooley_tukey_fft(const std::vector<std::complex<float>>& input) {
    std::vector<std::complex<float>> output(input.size());
    ct_fft(input.data(), output.data(), input.size(), 1);
    return output;
}

std::vector<std::complex<float>> inverse_cooley_tukey_fft(const std::vector<std::complex<float>>& input) {
    std::vector<std::complex<float>> output(input.size());
    std::vector<std::complex<float>> input_cpy(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        input_cpy.at(i) = std::conj(input.at(i));
    }
    ct_fft(input_cpy.data(), output.data(), input.size(), 1);
    for (size_t i = 0; i < output.size(); ++i) {
        output.at(i) = std::conj(output.at(i)) / static_cast<float>(output.size());
    }
    return output;
}

void cooley_tukey_fft_mag(const std::vector<float>& input, std::vector<float>& mag) {
    std::vector<std::complex<float>> output(input.size());
    ct_fft(input.data(), output.data(), input.size(), 1);
    std::transform(std::execution::par, output.begin(), output.begin() + mag.size(), mag.begin(), [](const std::complex<float>& c) {
        return std::sqrt(c.real() * c.real() + c.imag() * c.imag());
        });
}

struct Ring_buffer {
    Ring_buffer(size_t capacity) {
        data = std::vector<float>(capacity, 0.0f);
    }
    void push(const std::vector<float>& values) {
        std::rotate(data.begin(), data.begin() + values.size(), data.end());
        std::copy(values.begin(), values.end(), data.end() - values.size());
    }

    std::vector<float> data;
};