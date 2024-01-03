#include "gba/gba.h"
#include "gba/flash_internal.h"
#include "string.h"

static const char AgbLibFlashVersion[] = "FLASH1M_V103";

static const struct FlashSetupInfo * const sSetupInfos[] =
{
    &MX29L010,
    &LE26FV10N1TS,
    &BOOTLEG_SRAM,
    &DefaultFlash
};

EWRAM_INIT void bytecopy(u8 *dst,u8 *src,int count)
{
    do
    {
        *dst++ = *src++;
    } while (--count);
}

u16 IdentifyFlash(void)
{
    u16 result;
    u16 flashId;
    u32 flash_size;
    const struct FlashSetupInfo * const *setupInfo;
    // First check for bootlegs
    FlashType = get_flash_type();
    if (FlashType != 0)
    {
        ProgramFlashByte = ProgramFlashByte_SRAM;
        ProgramFlashSector = ProgramFlashSector_SRAM;
        EraseFlashChip = EraseFlashChip_SRAM;
        EraseFlashSector = EraseFlashSector_SRAM;
        WaitForFlashWrite = WaitForFlashWrite_SRAM;
        gFlashMaxTime = BOOTLEG_SRAM.maxTime;
        gFlash = &BOOTLEG_SRAM.type;
        // Determine the size of the flash chip by checking for ROM loops,
        // then set the SRAM storage area 0x40000 bytes before the end.
        // This is due to different sector sizes of different flash chips,
        // and should hopefully cover all cases.
        if (memcmp(AGB_ROM+4, AGB_ROM+4+0x400000, 0x40) == 0) {
            flash_size = 0x400000;
        } else if (memcmp(AGB_ROM+4, AGB_ROM+4+0x800000, 0x40) == 0) {
            flash_size = 0x800000;
        } else if (memcmp(AGB_ROM+4, AGB_ROM+4+0x1000000, 0x40) == 0) {
            flash_size = 0x1000000;
        } else {
            flash_size = 0x2000000;
        }
        if (FlashSRAMArea == 0) {
            FlashSRAMArea = flash_size - 0x40000;
        }
        bytecopy(AGB_SRAM, ((u8*)AGB_ROM+FlashSRAMArea), AGB_SRAM_SIZE);
        return 0;
    }

    REG_WAITCNT = (REG_WAITCNT & ~WAITCNT_SRAM_MASK) | WAITCNT_SRAM_8;

    flashId = ReadFlashId();

    setupInfo = sSetupInfos;
    result = 1;

    for (;;)
    {
        if ((*setupInfo)->type.ids.separate.makerId == 0)
            break;

        if (flashId == (*setupInfo)->type.ids.joined)
        {
            result = 0;
            break;
        }

        setupInfo++;
    }

    ProgramFlashByte = (*setupInfo)->programFlashByte;
    ProgramFlashSector = (*setupInfo)->programFlashSector;
    EraseFlashChip = (*setupInfo)->eraseFlashChip;
    EraseFlashSector = (*setupInfo)->eraseFlashSector;
    WaitForFlashWrite = (*setupInfo)->WaitForFlashWrite;
    gFlashMaxTime = (*setupInfo)->maxTime;
    gFlash = &(*setupInfo)->type;

    return result;
}

u16 WaitForFlashWrite_Common(u8 phase, u8 *addr, u8 lastData)
{
    u16 result = 0;
    u8 status;

    StartFlashTimer(phase);

    while ((status = PollFlashStatus(addr)) != lastData)
    {
        if (status & 0x20)
        {
            // The write operation exceeded the flash chip's time limit.

            if (PollFlashStatus(addr) == lastData)
                break;

            FLASH_WRITE(0x5555, 0xF0);
            result = phase | 0xA000u;
            break;
        }

        if (gFlashTimeoutFlag)
        {
            if (PollFlashStatus(addr) == lastData)
                break;

            FLASH_WRITE(0x5555, 0xF0);
            result = phase | 0xC000u;
            break;
        }
    }

    StopFlashTimer();

    return result;
}
