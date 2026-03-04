#pragma once
#include <SDL2/SDL.h>
#include <vector>
#include <cmath>
#include <mutex>
#include <algorithm>
#include <iostream>

// ── Voice: one playing sound ──────────────────────────────────────────────
struct Voice {
    enum class Wave { Sine, Square, Triangle, Sawtooth, Noise };

    Wave  wave      = Wave::Sine;
    float freq      = 440.0f;   // Hz, can change per-sample for sweeps
    float freqEnd   = 440.0f;   // target freq (for sweeps)
    float amp       = 0.3f;     // 0..1
    float ampEnd    = 0.0f;     // fade target
    float pan       = 0.0f;     // -1=left, 0=center, 1=right
    int   samplePos = 0;
    int   totalSamples = 0;     // 0 = infinite (ambient)
    bool  done      = false;

    // Per-sample generation
    float nextSample(int sampleRate) {
        if (done) return 0.0f;
        float t  = (float)samplePos / sampleRate;
        float progress = (totalSamples > 0)
            ? (float)samplePos / totalSamples : 0.0f;

        // Interpolate freq and amp
        float f = freq + (freqEnd - freq) * progress;
        float a = amp  + (ampEnd  - amp)  * progress;

        float phase = 2.0f * 3.14159265f * f * t;
        float s = 0.0f;
        switch (wave) {
        case Wave::Sine:
            s = sinf(phase);
            break;
        case Wave::Square:
            s = sinf(phase) >= 0 ? 1.0f : -1.0f;
            break;
        case Wave::Triangle:
            s = 2.0f * fabsf(2.0f * (f * t - floorf(f * t + 0.5f))) - 1.0f;
            break;
        case Wave::Sawtooth:
            s = 2.0f * (f * t - floorf(f * t + 0.5f));
            break;
        case Wave::Noise:
            s = ((float)(rand() % 2000) / 1000.0f) - 1.0f;
            break;
        }

        samplePos++;
        if (totalSamples > 0 && samplePos >= totalSamples) done = true;
        return s * a;
    }
};

// ── SFX: global sound system ──────────────────────────────────────────────
class SFXSystem {
public:
    static constexpr int SAMPLE_RATE  = 44100;
    static constexpr int CHANNELS     = 2;      // stereo
    static constexpr int BUFFER_SIZE  = 512;

    bool init() {
        SDL_AudioSpec want{}, got{};
        want.freq     = SAMPLE_RATE;
        want.format   = AUDIO_F32SYS;
        want.channels = CHANNELS;
        want.samples  = BUFFER_SIZE;
        want.callback = audioCallback;
        want.userdata = this;

        deviceId_ = SDL_OpenAudioDevice(nullptr, 0, &want, &got, 0);
        if (deviceId_ == 0) {
            std::cerr << "[SFX] SDL_OpenAudioDevice: " << SDL_GetError() << "\n";
            return false;
        }
        SDL_PauseAudioDevice(deviceId_, 0);  // start playing
        std::cout << "[SFX] Audio device opened. Rate=" << got.freq << "\n";
        return true;
    }

    void destroy() {
        if (deviceId_) {
            SDL_CloseAudioDevice(deviceId_);
            deviceId_ = 0;
        }
    }

    float masterVolume = 0.6f;  // 0..1, adjustable from settings

    // ── Sound triggers ───────────────────────────────────────────────────

    // Water splash at game start
    void splash() {
        addVoice({Wave::Noise, 0, 0, 0.25f, 0.0f, 0, 0, ms(180)});
        addVoice({Wave::Sine, 800, 200, 0.18f, 0.0f, 0, 0, ms(250)});
        addVoice({Wave::Sine, 400, 100, 0.12f, 0.0f, 0, 0, ms(300)});
    }

    // Soft "gulp" — diamond swallowed
    void swallow() {
        addVoice({Wave::Sine,     600, 180, 0.22f, 0.0f, 0, 0, ms(220)});
        addVoice({Wave::Triangle, 300, 120, 0.15f, 0.0f, 0, 0, ms(180)});
        addVoice({Wave::Noise,    0,   0,   0.08f, 0.0f, 0, 0, ms(80)});
    }

    // Tick — each second in last 5s
    void tick() {
        addVoice({Wave::Square, 880, 880, 0.15f, 0.0f, 0, 0, ms(60)});
        addVoice({Wave::Sine,   440, 440, 0.10f, 0.0f, 0, 0, ms(80)});
    }

    // Urgent tick — last 2s
    void tickUrgent() {
        addVoice({Wave::Square,   1320, 1320, 0.22f, 0.0f, 0, 0, ms(60)});
        addVoice({Wave::Sawtooth,  660,  660, 0.12f, 0.0f, 0, 0, ms(80)});
    }

    // Time up — low thud + buzz
    void timeUp() {
        addVoice({Wave::Sine,     80,  40,  0.35f, 0.0f, 0, 0, ms(400)});
        addVoice({Wave::Sawtooth, 160, 60,  0.20f, 0.0f, 0, 0, ms(350)});
        addVoice({Wave::Noise,    0,   0,   0.12f, 0.0f, 0, 0, ms(200)});
    }

    // Spread — whoosh sweep
    void spread() {
        addVoice({Wave::Sine,  200, 1200, 0.18f, 0.0f, 0, 0, ms(400)});
        addVoice({Wave::Noise, 0,   0,    0.08f, 0.0f, 0, 0, ms(300)});
    }

    // Correct — rising arpeggio (3 staggered notes)
    void correct() {
        addVoiceDelayed({Wave::Sine, 523, 523, 0.25f, 0.0f, 0, 0, ms(180)}, 0);
        addVoiceDelayed({Wave::Sine, 659, 659, 0.25f, 0.0f, 0, 0, ms(180)}, ms(120));
        addVoiceDelayed({Wave::Sine, 784, 784, 0.28f, 0.0f, 0, 0, ms(250)}, ms(240));
        addVoiceDelayed({Wave::Sine,1047,1047, 0.22f, 0.0f, 0, 0, ms(300)}, ms(360));
    }

    // Wrong — descending buzz
    void wrong() {
        addVoice({Wave::Sawtooth, 300, 80,  0.28f, 0.0f, 0, 0, ms(400)});
        addVoice({Wave::Square,   150, 60,  0.18f, 0.0f, 0, 0, ms(350)});
        addVoice({Wave::Noise,    0,   0,   0.10f, 0.0f, 0, 0, ms(150)});
    }

    // Hint — soft chime
    void hint() {
        addVoice({Wave::Sine,  1047, 1047, 0.18f, 0.0f, 0, 0, ms(300)});
        addVoice({Wave::Sine,  1319, 1319, 0.12f, 0.0f, 0, 0, ms(250)});
        addVoice({Wave::Sine,   523,  523, 0.10f, 0.0f, 0, 0, ms(350)});
    }

    // Start/stop ambient background (gentle bubbles)
    void startAmbient() { ambientOn_ = true; }
    void stopAmbient()  { ambientOn_ = false; }

private:
    SDL_AudioDeviceID deviceId_ = 0;
    std::vector<Voice>      voices_;
    std::mutex              mtx_;
    bool                    ambientOn_ = false;
    float                   ambientPhase_ = 0.0f;
    int                     bubbleTimer_  = 0;

    using Wave = Voice::Wave;

    int ms(int milliseconds) const {
        return (SAMPLE_RATE * milliseconds) / 1000;
    }

    void addVoice(Voice v) {
        std::lock_guard<std::mutex> lock(mtx_);
        voices_.push_back(v);
    }

    // Delayed voice: starts after `delaySamples` have elapsed.
    // Implemented by pre-advancing samplePos to a negative start.
    void addVoiceDelayed(Voice v, int delaySamples) {
        v.samplePos = -delaySamples;
        v.totalSamples += delaySamples;
        std::lock_guard<std::mutex> lock(mtx_);
        voices_.push_back(v);
    }

    // Ambient: gentle sine waves modulated slowly, occasional noise pops
    float nextAmbientSample(int idx) {
        if (!ambientOn_) return 0.0f;
        ambientPhase_ += 2.0f * 3.14159f * 60.0f / SAMPLE_RATE;
        float base = sinf(ambientPhase_) * 0.018f;

        // Rare bubble pop
        bubbleTimer_++;
        if (bubbleTimer_ > SAMPLE_RATE * 2 + rand() % (SAMPLE_RATE * 3)) {
            bubbleTimer_ = 0;
        }
        float bubble = (bubbleTimer_ < 400)
            ? sinf(bubbleTimer_ * 0.08f) * expf(-bubbleTimer_ * 0.005f) * 0.04f
            : 0.0f;

        return (base + bubble) * masterVolume;
    }

    static void audioCallback(void* userdata, Uint8* stream, int len) {
        auto* sfx = static_cast<SFXSystem*>(userdata);
        int numFrames = len / (sizeof(float) * CHANNELS);
        float* out = reinterpret_cast<float*>(stream);
        std::fill(out, out + numFrames * CHANNELS, 0.0f);

        std::lock_guard<std::mutex> lock(sfx->mtx_);

        for (int i = 0; i < numFrames; i++) {
            float ambient = sfx->nextAmbientSample(i);
            float mixL = ambient, mixR = ambient;

            for (auto& v : sfx->voices_) {
                if (v.done) continue;
                // Handle negative samplePos (delay countdown)
                if (v.samplePos < 0) { v.samplePos++; continue; }
                float s = v.nextSample(SAMPLE_RATE) * sfx->masterVolume;
                float panL = (v.pan <= 0) ? 1.0f : 1.0f - v.pan;
                float panR = (v.pan >= 0) ? 1.0f : 1.0f + v.pan;
                mixL += s * panL;
                mixR += s * panR;
            }

            // Soft clip to prevent distortion
            out[i * 2 + 0] = tanhf(mixL);
            out[i * 2 + 1] = tanhf(mixR);
        }

        // Remove finished voices
        sfx->voices_.erase(
            std::remove_if(sfx->voices_.begin(), sfx->voices_.end(),
                           [](const Voice& v){ return v.done; }),
            sfx->voices_.end());
    }
};

// Global singleton — include this header once in main.cpp,
// access everywhere via extern declaration or direct include.
inline SFXSystem SFX;
