#pragma once

// 统一输入事件：鼠标和手势都转换成这个结构
// 游戏逻辑永远只处理 InputEvent，不知道来源
struct InputEvent {
    enum class Type {
        NONE,
        // 鼠标事件
        MOUSE_MOVE,
        MOUSE_DOWN,
        MOUSE_UP,
        // 手势事件（Phase 3 实装）
        GESTURE_POINT,      // 食指指向某位置
        GESTURE_SELECT,     // 捏合手势 = 点击
        GESTURE_CANCEL,     // 手离开画面
    };

    Type type = Type::NONE;
    float x = 0.0f;        // 归一化坐标 [0.0, 1.0]
    float y = 0.0f;
    float confidence = 1.0f;  // 手势置信度；鼠标事件固定为 1.0
    bool handled = false;
};
