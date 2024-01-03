#ifndef GUARD_GBA_FLASH_INTERNAL_H
#define GUARD_GBA_FLASH_INTERNAL_H

#define FLASH_BASE ((u8 *)0xE000000)

#define FLASH_WRITE(addr, data) ((*(vu8 *)(FLASH_BASE + (addr))) = (data))

#define FLASH_ROM_SIZE_1M 131072 // 1 megabit ROM

#define SECTORS_PER_BANK 32

#define AGB_ROM  ((u8*)0x8000000)
#define AGB_SRAM ((u8*)0xE000000)
#define SRAM_SIZE 64
#define AGB_SRAM_SIZE SRAM_SIZE*1024
#define _FLASH_WRITE(pa, pd) { *(((u16 *)AGB_ROM)+((pa)/2)) = pd; __asm("nop"); }

struct FlashSector
{
    u32 size;
    u8 shift;
    u16 count;
    u16 top;
};

struct FlashType {
    u32 romSize;
    struct FlashSector sector;
    u16 wait[2]; // game pak bus read/write wait

    // TODO: add support for anonymous unions/structs if possible
    union {
        struct {
        u8 makerId;
        u8 deviceId;
        } separate;
        u16 joined;
    } ids;
};

struct FlashSetupInfo
{
    u16 (*programFlashByte)(u16, u32, u8);
    u16 (*programFlashSector)(u16, u8 *);
    u16 (*eraseFlashChip)(void);
    u16 (*eraseFlashSector)(u16);
    u16 (*WaitForFlashWrite)(u8, u8 *, u8);
    const u16 *maxTime;
    struct FlashType type;
};

extern u16 gFlashNumRemainingBytes;

extern u16 (*ProgramFlashByte)(u16, u32, u8);
extern u16 (*ProgramFlashSector)(u16, u8 *);
extern u16 (*EraseFlashChip)(void);
extern u16 (*EraseFlashSector)(u16);
extern u16 (*WaitForFlashWrite)(u8, u8 *, u8);
extern const u16 *gFlashMaxTime;
extern const struct FlashType *gFlash;

extern u8 (*PollFlashStatus)(u8 *);
extern u8 gFlashTimeoutFlag;

extern const struct FlashSetupInfo MX29L010;
extern const struct FlashSetupInfo LE26FV10N1TS;
extern const struct FlashSetupInfo BOOTLEG_SRAM;
extern const struct FlashSetupInfo DefaultFlash;

void SwitchFlashBank(u8 bankNum);
u16 ReadFlashId(void);
void StartFlashTimer(u8 phase);
void SetReadFlash1(u16 *dest);
void StopFlashTimer(void);
void ReadFlash(u16 sectorNum, u32 offset, u8 *dest, u32 size);

u16 WaitForFlashWrite_Common(u8 phase, u8 *addr, u8 lastData);
u16 WaitForFlashWrite_SRAM(u8 phase, u8 *addr, u8 lastData);

u16 EraseFlashChip_MX(void);
u16 EraseFlashSector_MX(u16 sectorNum);
u16 ProgramFlashByte_MX(u16 sectorNum, u32 offset, u8 data);
u16 ProgramFlashSector_MX(u16 sectorNum, u8 *src);

u16 EraseFlashChip_SRAM(void);
u16 EraseFlashSector_SRAM(u16 sectorNum);
u16 ProgramFlashByte_SRAM(u16 sectorNum, u32 offset, u8 data);
u16 ProgramFlashSector_SRAM(u16 sectorNum, u8 *src);

extern u32 get_flash_type();
extern void save_sram_FLASH();
extern u32 FlashType;
extern u32 FlashSRAMArea;
extern const u16 mxMaxTime[];

#endif // GUARD_GBA_FLASH_INTERNAL_H
