#include <cstdint>
#include <stdexcept>
#include <string>
#include <prx/libc/include/General.hpp>

#include "SceShaders.hpp"
#include "prx/libSceAgc/Shader/include/ShaderConstants.hpp"

extern "C" {

int APS5_VABI sceAgcGetGsPrimPayload(std::uint32_t* payload, const Shader* gs) {
    if (payload == nullptr || gs == nullptr) {
        throw std::runtime_error(std::string(__func__) + ": payload or gs is null");
    }
    if (gs->num_cx_registers != 0 && gs->cx_registers == nullptr) {
        throw std::runtime_error(std::string(__func__) + ": gs lists context registers but has no register array");
    }
    *payload = 0;
    for (std::uint32_t i = 0; i < gs->num_cx_registers; ++i) {
        if (gs->cx_registers[i].offset != ShaderRegs::SPI_SHADER_IDX_FORMAT) continue;
        if ((gs->cx_registers[i].value & 0xFu) == 2u) *payload = 8;
        break;
    }
    return 0;
}

}
