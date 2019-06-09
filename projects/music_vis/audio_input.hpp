#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <portaudio/portaudio.h>
#include <vector>
#include <mutex>

namespace impl {
    /* This routine will be called by the PortAudio engine when audio is needed.
    ** It may be called at interrupt level on some machines so don't do anything
    ** that could mess up the system like calling malloc() or free().
    */

    std::mutex audio_data_mutex;
    static int record_callback(const void* inputBuffer, void*, unsigned long framesPerBuffer,
            const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData) {
        auto data = reinterpret_cast<std::vector<float>*>(userData);
        const float* rptr = reinterpret_cast<const float*>(inputBuffer);
        int finished;
        auto framesLeft = data->size();
        finished = paContinue;
        std::lock_guard<std::mutex> lock(audio_data_mutex);
        if (inputBuffer == nullptr) {
            std::fill(data->begin(), data->end(), 0.0f);
        }
        else {
            std::memcpy(data->data(), rptr, data->size() * sizeof(float));
        }
        return finished;
    };

    class Recorder {
    public:
        Recorder(float fs, unsigned int block_length, int device_id = -1, int channels = 1)
            : _block_length(block_length), _channels(channels) {
            auto err = Pa_Initialize();
            data.resize(block_length * channels, 0.0f);
            start_recording_input(fs, block_length, device_id, channels);
        }
        ~Recorder() {
            Pa_CloseStream(stream);
            Pa_Terminate();
        }
        bool receive() {
            return Pa_IsStreamActive(stream) == 1;
        }
        inline void start_recording_input(float fs, unsigned int block_length, int device_id = -1, int channels = 1) {
            PaStreamParameters inputParameters;
            inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
            inputParameters.channelCount = 1;                    /* stereo input */
            inputParameters.sampleFormat = paFloat32;
            inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
            inputParameters.hostApiSpecificStreamInfo = nullptr;
            auto err = Pa_OpenStream(&stream, &inputParameters, nullptr, fs, block_length, paClipOff, record_callback, &data);
            err = Pa_StartStream(stream);
        }
        std::vector<float> data;
    private:
        PaStream* stream;
        unsigned int _block_length;
        int _channels;
    };

    
}
