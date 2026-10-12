#include "SceShaders.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libSceAgc/Shader/include/ShaderConstants.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <stdexcept>

extern "C" int APS5_VABI sceAgcGetGsPrimPayload(std::uint32_t* payload, const Shader* gs);

namespace {

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template <typename TAction>
void expectFailure(TAction action) {
    try {
        action();
    } catch (const std::exception& error) {
        check(error.what()[0] != '\0', "empty exception message");
        return;
    }
    throw std::runtime_error("expected an exception");
}

template <std::size_t Count>
std::uint32_t payloadFor(std::array<ShaderRegister, Count>& registers) {
    Shader gs{};
    gs.cx_registers = registers.data();
    gs.num_cx_registers = static_cast<std::uint8_t>(Count);
    std::uint32_t payload = 0xccccccccu;
    check(sceAgcGetGsPrimPayload(&payload, &gs) == 0, "sceAgcGetGsPrimPayload failed");
    return payload;
}

void testPayload() {
    std::array<ShaderRegister, 2> twoComponents{{{ShaderRegs::GE_NGG_SUBGRP_CNTL, 2u}, {ShaderRegs::SPI_SHADER_IDX_FORMAT, 2u}}};
    check(payloadFor(twoComponents) == 8u, "a two-component index export did not report an 8-byte payload");
    std::array<ShaderRegister, 1> upperBits{{{ShaderRegs::SPI_SHADER_IDX_FORMAT, 0xfffffff2u}}};
    check(payloadFor(upperBits) == 8u, "bits above the index export format changed the payload");
    for (const std::uint32_t format : {0u, 1u, 3u, 4u, 0xfu}) {
        std::array<ShaderRegister, 1> other{{{ShaderRegs::SPI_SHADER_IDX_FORMAT, format}}};
        check(payloadFor(other) == 0u, "an index export format other than two components reported a payload");
    }
    std::array<ShaderRegister, 2> firstWins{{{ShaderRegs::SPI_SHADER_IDX_FORMAT, 1u}, {ShaderRegs::SPI_SHADER_IDX_FORMAT, 2u}}};
    check(payloadFor(firstWins) == 0u, "a later duplicate of the index format register was used");
    std::array<ShaderRegister, 2> missing{{{ShaderRegs::GE_NGG_SUBGRP_CNTL, 2u}, {ShaderRegs::VGT_GS_ONCHIP_CNTL, 2u}}};
    check(payloadFor(missing) == 0u, "a shader without the index format register reported a payload");
}

void testInvalid() {
    Shader empty{};
    std::uint32_t payload = 0xccccccccu;
    check(sceAgcGetGsPrimPayload(&payload, &empty) == 0 && payload == 0u, "a shader without context registers did not report 0");
    expectFailure([&] { sceAgcGetGsPrimPayload(nullptr, &empty); });
    expectFailure([&] { sceAgcGetGsPrimPayload(&payload, nullptr); });
    Shader dangling{};
    dangling.num_cx_registers = 1;
    payload = 0xccccccccu;
    expectFailure([&] { sceAgcGetGsPrimPayload(&payload, &dangling); });
    check(payload == 0xccccccccu, "a rejected shader changed the payload");
}

}

int main() {
    try {
        testPayload();
        testInvalid();
        std::puts("AGC GS primitive payload tests passed");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
