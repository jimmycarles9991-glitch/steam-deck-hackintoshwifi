#ifndef RTW88_HPP
#define RTW88_HPP

#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/pci/IOPCIDevice.h>

// Register offsets, from drivers/net/wireless/realtek/rtw88/reg.h
#define REG_SYS_FUNC_EN   0x0002
#define REG_SYS_CFG1      0x00F0
#define REG_SYS_CFG2      0x00FC
#define REG_SYS_CFG3      0x1000
#define REG_SYS_CFG4      0x1010

// rtw88 uses BAR2 for 8822CE (RTW_PCI_BAR_ID == 2)
#define RTW_PCI_BAR_OFFSET 0x18

#define RTW_LOG(fmt, ...) IOLog("rtw88: " fmt "\n", ##__VA_ARGS__)

class RTW88 : public IOService {
    OSDeclareDefaultStructors(RTW88)

public:
    bool start(IOService *provider) override;
    void stop(IOService *provider) override;
    IOService *probe(IOService *provider, SInt32 *score) override;

private:
    IOPCIDevice       *pci   = nullptr;
    IOMemoryMap       *bar   = nullptr;
    volatile uint8_t  *regs  = nullptr;

    inline uint8_t  rd8(uint32_t off)  { return regs[off]; }
    inline uint16_t rd16(uint32_t off) { return *(volatile uint16_t *)(regs + off); }
    inline uint32_t rd32(uint32_t off) { return *(volatile uint32_t *)(regs + off); }
    inline void wr8(uint32_t off, uint8_t v)   { regs[off] = v; }
    inline void wr32(uint32_t off, uint32_t v) { *(volatile uint32_t *)(regs + off) = v; }

    void dumpChipInfo();
};

#endif
