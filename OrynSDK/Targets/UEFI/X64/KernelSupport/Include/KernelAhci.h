#ifndef ORYN_KERNEL_AHCI_H
#define ORYN_KERNEL_AHCI_H

#include "KernelBlockDevice.h"
#include "KernelPci.h"

#define ORYN_AHCI_MAX_CONTROLLERS 8U
#define ORYN_AHCI_MAX_PORTS 32U
#define ORYN_AHCI_SECTOR_SIZE 512U

#define ORYN_AHCI_PORT_TYPE_NONE 0U
#define ORYN_AHCI_PORT_TYPE_SATA 1U
#define ORYN_AHCI_PORT_TYPE_SATAPI 2U
#define ORYN_AHCI_PORT_TYPE_SEMB 3U
#define ORYN_AHCI_PORT_TYPE_PORT_MULTIPLIER 4U

#define ORYN_AHCI_CAP_S64A 0x80000000U
#define ORYN_AHCI_CAP_SNCQ 0x40000000U
#define ORYN_AHCI_GHC_AE 0x80000000U
#define ORYN_AHCI_PxCMD_ST 0x00000001U
#define ORYN_AHCI_PxCMD_FRE 0x00000010U
#define ORYN_AHCI_PxSSTS_DET_MASK 0x0000000FU
#define ORYN_AHCI_PxSSTS_IPM_MASK 0x00000F00U
#define ORYN_AHCI_SIG_SATA 0x00000101U
#define ORYN_AHCI_SIG_ATAPI 0xEB140101U
#define ORYN_AHCI_SIG_SEMB 0xC33C0101U
#define ORYN_AHCI_SIG_PM 0x96690101U

typedef struct OrynAhciHbaMemory
{
    volatile unsigned int Capabilities;
    volatile unsigned int GlobalHostControl;
    volatile unsigned int InterruptStatus;
    volatile unsigned int PortsImplemented;
    volatile unsigned int Version;
    volatile unsigned int CccControl;
    volatile unsigned int CccPorts;
    volatile unsigned int EnclosureManagementLocation;
    volatile unsigned int EnclosureManagementControl;
    volatile unsigned int CapabilitiesExtended;
    volatile unsigned int BiosOsHandoff;
    volatile unsigned char Reserved[116];
    volatile unsigned char Vendor[96];
} OrynAhciHbaMemory;

typedef struct OrynAhciHbaPort
{
    volatile unsigned int CommandListBase;
    volatile unsigned int CommandListBaseUpper;
    volatile unsigned int FisBase;
    volatile unsigned int FisBaseUpper;
    volatile unsigned int InterruptStatus;
    volatile unsigned int InterruptEnable;
    volatile unsigned int CommandAndStatus;
    volatile unsigned int Reserved0;
    volatile unsigned int TaskFileData;
    volatile unsigned int Signature;
    volatile unsigned int SataStatus;
    volatile unsigned int SataControl;
    volatile unsigned int SataError;
    volatile unsigned int SataActive;
    volatile unsigned int CommandIssue;
    volatile unsigned int SataNotification;
    volatile unsigned int FisBasedSwitching;
    volatile unsigned int Reserved1[11];
    volatile unsigned int Vendor[4];
} OrynAhciHbaPort;

typedef struct OrynAhciPortRecord
{
    unsigned int ControllerIndex;
    unsigned int PortNumber;
    unsigned int Implemented;
    unsigned int DevicePresent;
    unsigned int PortType;
    unsigned int Signature;
    unsigned int SataStatus;
    unsigned int CommandStatus;
    unsigned int BlockDeviceRegistered;
    unsigned long long BlockCount;
    OrynKernelBlockDevice BlockDevice;
} OrynAhciPortRecord;

typedef struct OrynAhciControllerRecord
{
    unsigned int Bus;
    unsigned int Device;
    unsigned int Function;
    unsigned int VendorId;
    unsigned int DeviceId;
    uint64_t AbarPhysical;
    unsigned int Capabilities;
    unsigned int GlobalHostControl;
    unsigned int PortsImplemented;
    unsigned int Version;
    unsigned int PortCount;
    unsigned int Enabled;
} OrynAhciControllerRecord;

typedef struct OrynAhciState
{
    unsigned int Initialized;
    unsigned int PciScanConsumed;
    unsigned int ControllersFound;
    unsigned int ControllersRecorded;
    unsigned int PortsImplemented;
    unsigned int PortsWithDevice;
    unsigned int SataPorts;
    unsigned int SatapiPorts;
    unsigned int SembPorts;
    unsigned int PortMultiplierPorts;
    unsigned int BlockDevicesPrepared;
    unsigned int BlockDevicesRegistered;
    unsigned int ReadRequests;
    unsigned int WriteRequests;
    unsigned int FlushRequests;
    unsigned int CommandIssueFoundationReady;
    unsigned int DmaPrdtFoundationReady;
    unsigned int ProofSyntheticControllerPassed;
    OrynAhciControllerRecord Controllers[ORYN_AHCI_MAX_CONTROLLERS];
    OrynAhciPortRecord Ports[ORYN_AHCI_MAX_PORTS];
} OrynAhciState;

void OrynAhciInitFromPci(void);
const OrynAhciState* OrynAhciGetState(void);
unsigned int OrynAhciClassifyPort(unsigned int signature, unsigned int sataStatus);
int OrynAhciRegisterPreparedBlockDevices(void);
int OrynAhciSelfTest(void);
void OrynAhciPrintProof(void);

#endif
