#include "SceTypes.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

extern "C" {
int32_t APS5_VABI sceSystemGestureOpen(int32_t input_type, const void* param);
int APS5_VABI sceSystemGestureClose(int32_t gesture_handle);
int APS5_VABI sceSystemGestureCreateTouchRecognizer(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer, int32_t type, const SystemGestureRectangle* rectangle, const void* param);
int APS5_VABI sceSystemGestureAppendTouchRecognizer(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer);
int APS5_VABI sceSystemGestureRemoveTouchRecognizer(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer);
int APS5_VABI sceSystemGestureResetTouchRecognizer(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer);
int APS5_VABI sceSystemGestureResetPrimitiveTouchRecognizer(int32_t gesture_handle);
int APS5_VABI sceSystemGestureUpdateTouchRecognizerRectangle(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer, const SystemGestureRectangle* rectangle);
int APS5_VABI sceSystemGestureGetTouchRecognizerInformation(int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer, SystemGestureTouchRecognizerInformation* information);
}

namespace {

constexpr int INVALID_ARGUMENT = static_cast<int>(0x80D10002);
constexpr int INVALID_HANDLE = static_cast<int>(0x80D10003);
constexpr int32_t TAP = 1;

void Require(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "%s\n", message);
        std::abort();
    }
}

template <typename TCall>
bool ThrowsRuntimeError(TCall call) {
    try {
        call();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

bool SameRectangle(const SystemGestureRectangle& left, const SystemGestureRectangle& right) {
    return left.x == right.x && left.y == right.y && left.width == right.width && left.height == right.height;
}

}

int main() {
    const int32_t handle = sceSystemGestureOpen(0, nullptr);
    Require(handle > 0, "open");
    auto* recognizer = new SystemGestureTouchRecognizer{};
    SystemGestureTouchRecognizerInformation information{};
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, recognizer, &information) == INVALID_ARGUMENT, "uncreated recognizer is rejected");
    Require(sceSystemGestureAppendTouchRecognizer(handle, recognizer) == INVALID_ARGUMENT, "uncreated recognizer cannot be appended");

    const SystemGestureRectangle area{10.f, 20.f, 300.f, 400.f, {}};
    Require(sceSystemGestureCreateTouchRecognizer(handle, recognizer, TAP, &area, nullptr) == 0, "create");
    std::memset(&information, 0xff, sizeof(information));
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, recognizer, &information) == 0, "information");
    Require(information.gesture_type == TAP && SameRectangle(information.rectangle, area) && information.updated_time == 0, "information reports type and area");
    Require(information.reserve[0] == 0 && information.reserve[255] == 0, "information is cleared");

    Require(sceSystemGestureAppendTouchRecognizer(handle, recognizer) == 0, "append");
    Require(ThrowsRuntimeError([&] { sceSystemGestureAppendTouchRecognizer(handle, recognizer); }), "double append throws");
    Require(sceSystemGestureResetTouchRecognizer(handle, recognizer) == 0, "reset");
    Require(sceSystemGestureResetPrimitiveTouchRecognizer(handle) == 0, "reset primitive");

    const SystemGestureRectangle moved{1.f, 2.f, 3.f, 4.f, {}};
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, recognizer, &moved) == 0, "update rectangle");
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, recognizer, &information) == 0, "information after update");
    Require(SameRectangle(information.rectangle, moved) && information.gesture_type == TAP, "rectangle updated");

    Require(sceSystemGestureRemoveTouchRecognizer(handle, recognizer) == 0, "remove");
    Require(ThrowsRuntimeError([&] { sceSystemGestureRemoveTouchRecognizer(handle, recognizer); }), "removing twice throws");
    Require(sceSystemGestureAppendTouchRecognizer(handle, recognizer) == 0, "append after remove");
    Require(sceSystemGestureRemoveTouchRecognizer(handle, recognizer) == 0, "remove again");

    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle, recognizer, nullptr) == INVALID_ARGUMENT, "null rectangle");
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, recognizer, nullptr) == INVALID_ARGUMENT, "null information");
    Require(sceSystemGestureAppendTouchRecognizer(handle, nullptr) == INVALID_ARGUMENT, "null recognizer");
    Require(sceSystemGestureAppendTouchRecognizer(handle + 1, recognizer) == INVALID_HANDLE, "bad handle on append");
    Require(sceSystemGestureRemoveTouchRecognizer(handle + 1, recognizer) == INVALID_HANDLE, "bad handle on remove");
    Require(sceSystemGestureResetTouchRecognizer(handle + 1, recognizer) == INVALID_HANDLE, "bad handle on reset");
    Require(sceSystemGestureResetPrimitiveTouchRecognizer(handle + 1) == INVALID_HANDLE, "bad handle on reset primitive");
    Require(sceSystemGestureUpdateTouchRecognizerRectangle(handle + 1, recognizer, &moved) == INVALID_HANDLE, "bad handle on update");
    Require(sceSystemGestureGetTouchRecognizerInformation(handle + 1, recognizer, &information) == INVALID_HANDLE, "bad handle on information");

    auto* whole = new SystemGestureTouchRecognizer{};
    Require(sceSystemGestureCreateTouchRecognizer(handle, whole, TAP, nullptr, nullptr) == 0, "create without rectangle");
    Require(sceSystemGestureGetTouchRecognizerInformation(handle, whole, &information) == 0, "information without rectangle");
    const SystemGestureRectangle empty{};
    Require(SameRectangle(information.rectangle, empty), "no rectangle reports zeros");

    delete whole;
    delete recognizer;
    Require(sceSystemGestureClose(handle) == 0, "close");
    std::puts("SystemGesture recognizer checks passed");
    return 0;
}
