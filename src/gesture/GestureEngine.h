#pragma once
#include "core/InputEvent.h"
#include <functional>
#include <atomic>
#include <thread>
#include <chrono>

// Phase 0: 桩实现，只开摄像头，不做手势识别
// Phase 3: 替换内部实现为 MediaPipe，接口不变

class GestureEngine {
public:
    using EventCallback = std::function<void(const InputEvent&)>;

    GestureEngine() : mRunning(false) {}

    ~GestureEngine() {
        stop();
    }

    // 禁止拷贝
    GestureEngine(const GestureEngine&) = delete;
    GestureEngine& operator=(const GestureEngine&) = delete;

    bool start(EventCallback callback) {
        mCallback = callback;
        mRunning = true;
        mThread = std::thread([this]() { loop(); });
        return true;
    }

    void stop() {
        mRunning = false;
        if (mThread.joinable()) {
            mThread.join();
        }
    }

    bool isRunning() const { return mRunning.load(); }

private:
    void loop() {
        // Phase 0: 没有 OpenCV，直接降级
        // Phase 3: 在这里跑 MediaPipe 推理循环
        mRunning = false;
    }

    EventCallback mCallback;
    std::atomic<bool> mRunning;
    std::thread mThread;
};
