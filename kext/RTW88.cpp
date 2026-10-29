#include "RTW88.hpp"

#define super IOService
OSDefineMetaClassAndStructors(RTW88, IOService)

IOService *RTW88::probe(IOService *provider, SInt32 *score)
{
    IOPCIDevice *dev = OSDynamicCast(IOPCIDevice, provider);
    if (!dev) return nullptr;
    uint16_t ven = dev->configRead16(kIOPCIConfigVendorID);
    uint16_t id  = dev->configRead16(kIOPCIConfigDeviceID);
    if (ven != 0x10ec || (id != 0xc822 && id != 0xc82f)) return nullptr;
    return super::probe(provider, score);
}

bool RTW88::start(IOService *provider)
{
    if (!super::start(provider)) return false;

    pci = OSDynamicCast(IOPCIDevice, provider);
    if (!pci) return false;
    pci->retain();
    if (!pci->open(this)) {
        RTW_LOG("pci open failed");
        pci->release(); pci = nullptr;
        return false;
    }

    // Wake to D0, enable MMIO + bus master
    pci->enablePCIPowerManagement(kPCIPMCSPowerStateD0);
    pci->setMemoryEnable(true);
    pci->setBusMasterEnable(true);

    // Kill ASPM L0s/L1 for bring-up
    uint16_t linkctl = 0;
    uint8_t pcie = 0;
    if (pci->findPCICapability(kIOPCIPCIExpressCapability, &pcie)) {
        linkctl = pci->configRead16(pcie + 0x10);
        pci->configWrite16(pcie + 0x10, linkctl & ~0x3);
        RTW_LOG("link ctl 0x%04x -> 0x%04x", linkctl, pci->configRead16(pcie + 0x10));
    }

    bar = pci->mapDeviceMemoryWithRegister(RTW_PCI_BAR_OFFSET);
    if (!bar) {
        RTW_LOG("BAR2 map failed");
        goto fail;
    }
    regs = (volatile uint8_t *)bar->getVirtualAddress();
    RTW_LOG("BAR2 mapped, len 0x%llx", (unsigned long long)bar->getLength());

    dumpChipInfo();

    registerService();
    return true;

fail:
    pci->close(this);
    pci->release(); pci = nullptr;
    return false;
}

void RTW88::stop(IOService *provider)
{
    if (bar) { bar->release(); bar = nullptr; regs = nullptr; }
    if (pci) {
        pci->setBusMasterEnable(false);
        pci->setMemoryEnable(false);
        pci->close(this);
        pci->release();
        pci = nullptr;
    }
    super::stop(provider);
}

void RTW88::dumpChipInfo()
{
    uint32_t cfg1 = rd32(REG_SYS_CFG1);
    uint32_t cfg2 = rd32(REG_SYS_CFG2);
    uint32_t cfg3 = rd32(REG_SYS_CFG3);
    // rtw88: cut_version = BIT_GET_CHIP_VER(cfg1) = (cfg1 >> 12) & 0xF
    RTW_LOG("SYS_CFG1=0x%08x (cut=%u) SYS_CFG2=0x%08x SYS_CFG3=0x%08x",
            cfg1, (cfg1 >> 12) & 0xF, cfg2, cfg3);
    RTW_LOG("SYS_FUNC_EN=0x%02x", rd8(REG_SYS_FUNC_EN));
    if (cfg1 == 0xFFFFFFFF)
        RTW_LOG("all-ones read: device not powered / link down");
}
