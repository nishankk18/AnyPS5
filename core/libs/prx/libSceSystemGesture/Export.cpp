#include <cstdint>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

// No touch input is emulated: recognizers exist but never report events.
static constexpr int32_t GESTURE_HANDLE = 1;
static constexpr int SCE_SYSTEM_GESTURE_ERROR_INVALID_ARGUMENT = static_cast<int>(0x80D10002);
static constexpr int SCE_SYSTEM_GESTURE_ERROR_INVALID_HANDLE = static_cast<int>(0x80D10003);
static constexpr int SCE_SYSTEM_GESTURE_ERROR_INDEX_OUT_OF_ARRAY = static_cast<int>(0x80D10005);
static constexpr std::uint64_t RECOGNIZER_MAGIC = 0x5453454741535041ull;

namespace {

struct RecognizerState {
    std::uint64_t magic;
    std::int32_t type;
    std::uint32_t appended;
    SystemGestureRectangle rectangle;
};

static_assert(sizeof(RecognizerState) <= sizeof(SystemGestureTouchRecognizer));

void StoreState(SystemGestureTouchRecognizer* recognizer, const RecognizerState& state) {
    std::memcpy(recognizer, &state, sizeof(state));
}

int LoadState(int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer, RecognizerState& state) {
    if (gesture_handle != GESTURE_HANDLE) return SCE_SYSTEM_GESTURE_ERROR_INVALID_HANDLE;
    if (!recognizer) return SCE_SYSTEM_GESTURE_ERROR_INVALID_ARGUMENT;
    std::memcpy(&state, recognizer, sizeof(state));
    return state.magic == RECOGNIZER_MAGIC ? 0 : SCE_SYSTEM_GESTURE_ERROR_INVALID_ARGUMENT;
}

}

extern "C" {

int APS5_VABI sceSystemGestureAppendTouchRecognizer(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer) {
    RecognizerState state;
    const int result = LoadState(gesture_handle, recognizer, state);
    if (result != 0) return result;
    if (state.appended) throw std::runtime_error(std::string(__func__) + ": recognizer is already appended");
    state.appended = 1;
    StoreState(recognizer, state);
    return 0;
}

int APS5_VABI sceSystemGestureClose(int32_t gesture_handle) {
    return gesture_handle == GESTURE_HANDLE ? 0 : SCE_SYSTEM_GESTURE_ERROR_INVALID_HANDLE;
}

int APS5_VABI sceSystemGestureCreateTouchRecognizer(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer, int32_t type, const SystemGestureRectangle* rectangle, const void* param) {
    (void)param;
    if (gesture_handle != GESTURE_HANDLE) return SCE_SYSTEM_GESTURE_ERROR_INVALID_HANDLE;
    if (!recognizer) return SCE_SYSTEM_GESTURE_ERROR_INVALID_ARGUMENT;
    RecognizerState state{RECOGNIZER_MAGIC, type, 0, {}};
    if (rectangle) state.rectangle = *rectangle;
    StoreState(recognizer, state);
    return 0;
}

int APS5_VABI sceSystemGestureFinalizePrimitiveTouchRecognizer(void) {
    return 0;
}

int APS5_VABI sceSystemGestureGetPrimitiveTouchEventByIndex(int32_t gesture_handle, uint32_t index, SystemGesturePrimitiveTouchEvent* event) {
 (void)gesture_handle;
 (void)index;
 (void)event;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemGestureGetPrimitiveTouchEventByPrimitiveID(int32_t gesture_handle, uint16_t primitiveId, SystemGesturePrimitiveTouchEvent* event) {
 (void)gesture_handle;
 (void)primitiveId;
 (void)event;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemGestureGetPrimitiveTouchEvents(int32_t gesture_handle, SystemGesturePrimitiveTouchEvent* event_buffer, uint32_t capacity_of_buffer, uint32_t* number_of_event) {
 (void)gesture_handle;
 (void)event_buffer;
 (void)capacity_of_buffer;
 (void)number_of_event;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemGestureGetPrimitiveTouchEventsCount(int32_t gesture_handle) {
 (void)gesture_handle;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemGestureGetTouchEventByEventID(int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer, uint32_t eventId, SystemGestureTouchEvent* event) {
 (void)gesture_handle;
 (void)recognizer;
 (void)eventId;
 (void)event;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemGestureGetTouchEventByIndex(int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer, uint32_t index, SystemGestureTouchEvent* event) {
    (void)gesture_handle;
    (void)recognizer;
    (void)index;
    (void)event;
    return SCE_SYSTEM_GESTURE_ERROR_INDEX_OUT_OF_ARRAY;
}

int APS5_VABI sceSystemGestureGetTouchEvents(int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer, SystemGestureTouchEvent* event_buffer, uint32_t capacity_of_buffer, uint32_t* number_of_event) {
 (void)gesture_handle;
 (void)recognizer;
 (void)event_buffer;
 (void)capacity_of_buffer;
 (void)number_of_event;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemGestureGetTouchEventsCount(int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer) {
    (void)recognizer;
    return gesture_handle == GESTURE_HANDLE ? 0 : SCE_SYSTEM_GESTURE_ERROR_INVALID_HANDLE;
}

int APS5_VABI sceSystemGestureGetTouchRecognizerInformation(int32_t gesture_handle, const SystemGestureTouchRecognizer* recognizer, SystemGestureTouchRecognizerInformation* information) {
    RecognizerState state;
    const int result = LoadState(gesture_handle, recognizer, state);
    if (result != 0) return result;
    if (!information) return SCE_SYSTEM_GESTURE_ERROR_INVALID_ARGUMENT;
    std::memset(information, 0, sizeof(*information));
    information->gesture_type = state.type;
    information->rectangle = state.rectangle;
    return 0;
}

int APS5_VABI sceSystemGestureInitializePrimitiveTouchRecognizer(const void* param) {
    (void)param;
    return 0;
}

int32_t APS5_VABI sceSystemGestureOpen(int32_t input_type, const void* param) {
    (void)input_type;
    (void)param;
    return GESTURE_HANDLE;
}

int APS5_VABI sceSystemGestureRemoveTouchRecognizer(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer) {
    RecognizerState state;
    const int result = LoadState(gesture_handle, recognizer, state);
    if (result != 0) return result;
    if (!state.appended) throw std::runtime_error(std::string(__func__) + ": recognizer is not appended");
    state.appended = 0;
    StoreState(recognizer, state);
    return 0;
}

int APS5_VABI sceSystemGestureResetPrimitiveTouchRecognizer(int32_t gesture_handle) {
    return gesture_handle == GESTURE_HANDLE ? 0 : SCE_SYSTEM_GESTURE_ERROR_INVALID_HANDLE;
}

int APS5_VABI sceSystemGestureResetTouchRecognizer(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer) {
    RecognizerState state;
    return LoadState(gesture_handle, recognizer, state);
}

int APS5_VABI sceSystemGestureUpdateAllTouchRecognizer(int32_t gesture_handle) {
 (void)gesture_handle;
 NotImplemented_nid_no_patch(__func__);
 return 0;
}

int APS5_VABI sceSystemGestureUpdatePrimitiveTouchRecognizer(int32_t gesture_handle, const void* param) {
    (void)param;
    return gesture_handle == GESTURE_HANDLE ? 0 : SCE_SYSTEM_GESTURE_ERROR_INVALID_HANDLE;
}

int APS5_VABI sceSystemGestureUpdateTouchRecognizer(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer) {
    (void)recognizer;
    return gesture_handle == GESTURE_HANDLE ? 0 : SCE_SYSTEM_GESTURE_ERROR_INVALID_HANDLE;
}

int APS5_VABI sceSystemGestureUpdateTouchRecognizerRectangle(int32_t gesture_handle, SystemGestureTouchRecognizer* recognizer, const SystemGestureRectangle* rectangle) {
    RecognizerState state;
    const int result = LoadState(gesture_handle, recognizer, state);
    if (result != 0) return result;
    if (!rectangle) return SCE_SYSTEM_GESTURE_ERROR_INVALID_ARGUMENT;
    state.rectangle = *rectangle;
    StoreState(recognizer, state);
    return 0;
}

}
