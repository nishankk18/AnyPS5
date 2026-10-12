#include "prx/libSceAgc/Patch/include/AcquireMem.hpp"

#include "prx/libSceAgc/Command/include/Packet.hpp"
#include <cstdint>
#include <cstddef>
#include "SceTypes.hpp"
#include "prx/libc/include/General.hpp"

extern "C" {

int APS5_VABI sceAgcAcquireMemSetEngine(std::uint32_t* cmd, std::uint32_t engine) {
    Agc::Command::ValidatePacket(cmd, 0x58u, 8, __func__);
    Agc::Command::CheckBits(engine, 1u, __func__);
    cmd[1] = (cmd[1] & 0x7fffffffu) | (engine << 31u);
    return 0;
}

}
