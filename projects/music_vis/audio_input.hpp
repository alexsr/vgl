#pragma once

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <portaudio/portaudio.h>
#include <vector>
#include <mutex>
#include <memory>
#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <mmdeviceapi.h>
#include <Endpointvolume.h>

namespace impl {

    static int record_callback(const void* inputBuffer, void*, unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData);

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
            auto err = Pa_OpenStream(&stream, &inputParameters, nullptr, fs, block_length, paClipOff, record_callback, this);
            err = Pa_StartStream(stream);
        }
        std::vector<float> data;
        std::mutex audio_data_mutex;
    private:
        PaStream* stream;
        unsigned int _block_length;
        int _channels;
    };

    static int record_callback(const void* inputBuffer, void*, unsigned long framesPerBuffer,
        const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData) {
        auto rec = reinterpret_cast<Recorder*>(userData);
        const float* rptr = reinterpret_cast<const float*>(inputBuffer);
        int finished;
        auto framesLeft = rec->data.size();
        finished = paContinue;
        std::lock_guard<std::mutex> lock(rec->audio_data_mutex);
        if (inputBuffer == nullptr) {
            std::fill(rec->data.begin(), rec->data.end(), 0.0f);
        }
        else {
            std::memcpy(rec->data.data(), rptr, rec->data.size() * sizeof(float));
        }
        return finished;
    };

    struct Volume_getter {
        Volume_getter() {
            // Get enumerator for audio endpoint devices.
            CoInitialize(nullptr);
            auto hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
                nullptr, CLSCTX_INPROC_SERVER,
                __uuidof(IMMDeviceEnumerator),
                (void**)& enumerator);
            // Get default audio-rendering device.
            hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
            hr = device->Activate(__uuidof(IAudioEndpointVolume),
                CLSCTX_ALL, nullptr, (void**)& _endpoint_volume);
        }

        ~Volume_getter() {
            /*delete _endpoint_volume;
            delete enumerator;
            delete device;*/
        }

        float get_volume() {
            float current_volume;
            auto hr = _endpoint_volume->GetMasterVolumeLevelScalar(&current_volume);
            return current_volume;
        }

    private:
        IMMDeviceEnumerator* enumerator = nullptr;
        IMMDevice* device = nullptr;
        IAudioEndpointVolume* _endpoint_volume;
    };

}
