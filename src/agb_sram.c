#include "gba/gba.h"
#include "gba/flash_internal.h"


EWRAM_INIT u16 stop_interrupts()
{
    u16 save = REG_IME;
    REG_IME = 0;
    return save;
}

EWRAM_INIT void resume_interrupts(u16 save)
{
    REG_IME = save;
}

u32 FlashType;
u32 FlashSRAMArea;

// This function will auto-detect four common
// types of reproduction flash cartridges.
// Must run in EWRAM because ROM data is
// not visible to the system while checking.
EWRAM_INIT u32 get_flash_type()
{
    u32 rom_data, data;
    u16 save_ime = REG_IME;
    u16 ie = REG_IE;
    save_ime = stop_interrupts();
    REG_IE = ie & 0xFFFE;

    rom_data = *(u32 *)AGB_ROM;

    // Type 1 or 4
    _FLASH_WRITE(0, 0xFF);
    _FLASH_WRITE(0, 0x90);
    data = *(u32 *)AGB_ROM;
    _FLASH_WRITE(0, 0xFF);
    if (rom_data != data) {
        // Check if the chip is responding to this command
        // which then needs a different write command later
        _FLASH_WRITE(0x59, 0x42);
        data = *(u8 *)(AGB_ROM+0xB2);
        _FLASH_WRITE(0x59, 0x96);
        _FLASH_WRITE(0, 0xFF);
        if (data != 0x96) {
            REG_IE = ie;
            resume_interrupts(save_ime);
            return 4;
        }
        REG_IE = ie;
        resume_interrupts(save_ime);
        return 1;
    }

    // Type 2
    _FLASH_WRITE(0, 0xF0);
    _FLASH_WRITE(0xAAA, 0xA9);
    _FLASH_WRITE(0x555, 0x56);
    _FLASH_WRITE(0xAAA, 0x90);
    data = *(u32 *)AGB_ROM;
    _FLASH_WRITE(0, 0xF0);
    if (rom_data != data) {
        REG_IE = ie;
        resume_interrupts(save_ime);
        return 2;
    }

    // Type 3
    _FLASH_WRITE(0, 0xF0);
    _FLASH_WRITE(0xAAA, 0xAA);
    _FLASH_WRITE(0x555, 0x55);
    _FLASH_WRITE(0xAAA, 0x90);
    data = *(u32 *)AGB_ROM;
    _FLASH_WRITE(0, 0xF0);
    if (rom_data != data) {
        REG_IE = ie;
        resume_interrupts(save_ime);
        return 3;
    }

    REG_IE = ie;
    resume_interrupts(save_ime);
    return 0;
}

// This function will issue a flash sector erase
// operation at the given sector address and then
// write 64 kilobytes of SRAM data to Flash ROM.
// Must run in EWRAM because ROM data is
// not visible to the system while erasing/writing.
EWRAM_INIT void flash_write(u8 flash_type, u32 sa)
{
    u16 ie;
    u16 save_ime;
    if (flash_type == 0) return;
    ie = REG_IE;
    save_ime = stop_interrupts();
    REG_IE = ie & 0xFFFE;

    if (flash_type == 1) {
        // Erase flash sector
        _FLASH_WRITE(sa, 0xFF);
        _FLASH_WRITE(sa, 0x60);
        _FLASH_WRITE(sa, 0xD0);
        _FLASH_WRITE(sa, 0x20);
        _FLASH_WRITE(sa, 0xD0);
        while (1) {
            __asm("nop");
            if (*(((u16 *)AGB_ROM)+(sa/2)) == 0x80) {
                break;
            }
        }
        _FLASH_WRITE(sa, 0xFF);

        // Write data
        for (int i=0; i<AGB_SRAM_SIZE; i+=2) {
            _FLASH_WRITE(sa+i, 0x40);
            _FLASH_WRITE(sa+i, (*(u8 *)(AGB_SRAM+i+1)) << 8 | (*(u8 *)(AGB_SRAM+i)));
            while (1) {
                __asm("nop");
                if (*(((u16 *)AGB_ROM)+(sa/2)) == 0x80) {
                    break;
                }
            }
        }
        _FLASH_WRITE(sa, 0xFF);

    } else if (flash_type == 2) {
        // Erase flash sector
        _FLASH_WRITE(sa, 0xF0);
        _FLASH_WRITE(0xAAA, 0xA9);
        _FLASH_WRITE(0x555, 0x56);
        _FLASH_WRITE(0xAAA, 0x80);
        _FLASH_WRITE(0xAAA, 0xA9);
        _FLASH_WRITE(0x555, 0x56);
        _FLASH_WRITE(sa, 0x30);
        while (1) {
            __asm("nop");
            if (*(((u16 *)AGB_ROM)+(sa/2)) == 0xFFFF) {
                break;
            }
        }
        _FLASH_WRITE(sa, 0xF0);

        // Write data
        for (int i=0; i<AGB_SRAM_SIZE; i+=2) {
            _FLASH_WRITE(0xAAA, 0xA9);
            _FLASH_WRITE(0x555, 0x56);
            _FLASH_WRITE(0xAAA, 0xA0);
            _FLASH_WRITE(sa+i, (*(u8 *)(AGB_SRAM+i+1)) << 8 | (*(u8 *)(AGB_SRAM+i)));
            while (1) {
                __asm("nop");
                if (*(((u16 *)AGB_ROM)+((sa+i)/2)) == ((*(u8 *)(AGB_SRAM+i+1)) << 8 | (*(u8 *)(AGB_SRAM+i)))) {
                    break;
                }
            }
        }
        _FLASH_WRITE(sa, 0xF0);

    } else if (flash_type == 3) {
        // Erase flash sector
        _FLASH_WRITE(sa, 0xF0);
        _FLASH_WRITE(0xAAA, 0xAA);
        _FLASH_WRITE(0x555, 0x55);
        _FLASH_WRITE(0xAAA, 0x80);
        _FLASH_WRITE(0xAAA, 0xAA);
        _FLASH_WRITE(0x555, 0x55);
        _FLASH_WRITE(sa, 0x30);
        while (1) {
            __asm("nop");
            if (*(((u16 *)AGB_ROM)+(sa/2)) == 0xFFFF) {
                break;
            }
        }
        _FLASH_WRITE(sa, 0xF0);

        // Write data
        for (int i=0; i<AGB_SRAM_SIZE; i+=2) {
            _FLASH_WRITE(0xAAA, 0xAA);
            _FLASH_WRITE(0x555, 0x55);
            _FLASH_WRITE(0xAAA, 0xA0);
            _FLASH_WRITE(sa+i, (*(u8 *)(AGB_SRAM+i+1)) << 8 | (*(u8 *)(AGB_SRAM+i)));
            while (1) {
                __asm("nop");
                if (*(((u16 *)AGB_ROM)+((sa+i)/2)) == ((*(u8 *)(AGB_SRAM+i+1)) << 8 | (*(u8 *)(AGB_SRAM+i)))) {
                    break;
                }
            }
        }
        _FLASH_WRITE(sa, 0xF0);

    } else if (flash_type == 4) {
        // Erase flash sector
        _FLASH_WRITE(sa, 0xFF);
        _FLASH_WRITE(sa, 0x60);
        _FLASH_WRITE(sa, 0xD0);
        _FLASH_WRITE(sa, 0x20);
        _FLASH_WRITE(sa, 0xD0);
        while (1) {
            __asm("nop");
            if ((*(((u16 *)AGB_ROM)+(sa/2)) & 0x80) == 0x80) {
                break;
            }
        }
        _FLASH_WRITE(sa, 0xFF);

        // Write data
        int c = 0;
        while (c < AGB_SRAM_SIZE) {
            _FLASH_WRITE(sa+c, 0xEA);
            while (1) {
                __asm("nop");
                if ((*(((u16 *)AGB_ROM)+((sa+c)/2)) & 0x80) == 0x80) {
                    break;
                }
            }
            _FLASH_WRITE(sa+c, 0x1FF);
            for (int i=0; i<1024; i+=2) {
                _FLASH_WRITE(sa+c+i, (*(u8 *)(AGB_SRAM+c+i+1)) << 8 | (*(u8 *)(AGB_SRAM+c+i)));
            }
            _FLASH_WRITE(sa+c, 0xD0);
            while (1) {
                __asm("nop");
                if ((*(((u16 *)AGB_ROM)+((sa+c)/2)) & 0x80) == 0x80) {
                    break;
                }
            }
            _FLASH_WRITE(sa+c, 0xFF);
            c += 1024;
        }
    }

    REG_IE = ie;
    resume_interrupts(save_ime);
}

void save_sram_FLASH()
{
    if (FlashType == 0) return;
    flash_write(FlashType, FlashSRAMArea);
}

const struct FlashSetupInfo BOOTLEG_SRAM =
{
    ProgramFlashByte_SRAM,
    ProgramFlashSector_SRAM,
    EraseFlashChip_SRAM,
    EraseFlashSector_SRAM,
    WaitForFlashWrite_SRAM,
    mxMaxTime,
    {
        64000,
        {
            4096,
            12,
            32,
            0
        },
        { 2, 1 },
    { { 0xFF, 0xFF } }
    }
};

EWRAM_INIT static u16 ProgramByte(u8 *src, u8 *dest)
{
    *dest = *src;
    return 0;
}

u16 ProgramFlashByte_SRAM(u16 sectorNum, u32 offset, u8 data)
{
    u8 *addr;
    if (offset >= gFlash->sector.size)
        return 0x8000;
    addr = AGB_SRAM + (sectorNum << gFlash->sector.shift) + offset;
    ProgramByte(addr, &data);
    return 0;
}

u16 ProgramFlashSector_SRAM(u16 sectorNum, u8 *src)
{
    u16 result;
    u8 *dest;

    result = EraseFlashSector_SRAM(sectorNum);
    if (result != 0)
        return result;

    gFlashNumRemainingBytes = gFlash->sector.size;
    dest = AGB_SRAM + (sectorNum << gFlash->sector.shift);

    while (gFlashNumRemainingBytes > 0)
    {
        result = ProgramByte(src, dest);

        if (result != 0)
            break;

        gFlashNumRemainingBytes--;
        src++;
        dest++;
    }

    return result;
}

EWRAM_INIT void bytefill(u8 *dest, u8 fill, u32 size)
{
    while(size--)
    {
        *dest = fill;
    }
}

u16 EraseFlashChip_SRAM(void)
{
    bytefill(AGB_SRAM, 0xFF, AGB_SRAM_SIZE);
    return 0;
}

u16 EraseFlashSector_SRAM(u16 sectorNum)
{
    u8 *addr;
    if (sectorNum >= gFlash->sector.count)
        return 0x80FF;
    addr = AGB_SRAM + (sectorNum << gFlash->sector.shift);
    bytefill(addr, 0xFF, 1);
    return 0;
}

u16 WaitForFlashWrite_SRAM(u8 phase, u8 *addr, u8 lastData)
{
    return 0;
}