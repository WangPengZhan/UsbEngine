#pragma once
#include <cstdint>
#include <string>

namespace dfu
{

enum DfuCommand : std::uint8_t
{
    DFU_DETACH,
    DFU_DNLOAD,
    DFU_UPLOAD,
    DFU_GETSTATUS,
    DFU_CLRSTATUS,
    DFU_GETSTATE,
    DFU_ABORT,
};

constexpr char kDfuStatusOk[] = "No error condition is present";
constexpr char kDfuStatusErrorTarget[] = "File is not targeted for use by this device";
constexpr char kDfuStatusErrorFile[] = "File is for this device but fails some vendor-specific test";
constexpr char kDfuStatusErrorWrite[] = "Device is unable to write memory";
constexpr char kDfuStatusErrorErase[] = "Memory erase function failed";
constexpr char kDfuStatusErrorCheckErased[] = "Memory erase check failed";
constexpr char kDfuStatusErrorProg[] = "Program memory function failed";
constexpr char kDfuStatusErrorVerify[] = "Programmed memory failed verification";
constexpr char kDfuStatusErrorAddress[] = "Cannot program memory due to received address that is out of range";
constexpr char kDfuStatusErrorNotDone[] = "Received DFU_DNLOAD with wLength = 0, but device does not think that it has all data yet";
constexpr char kDfuStatusErrorFirmware[] = "Device's firmware is corrupt. It cannot return to run-time operations";
constexpr char kDfuStatusErrorVendor[] = "Vendor specific error";
constexpr char kDfuStatusErrorUsbr[] = "Device detected unexpected USB reset signalling";
constexpr char kDfuStatusErrorPor[] = "Device detected unexpected power on reset";
constexpr char kDfuStatusErrorUnknown[] = "Unknown error";
constexpr char kDfuStatusErrorStalledPkt[] = "Device stalled an unexpected request";

enum DfuStatus : std::uint8_t
{
    OK,
    ERROR_TARGET,
    ERROR_FILE,
    ERROR_WRITE,
    ERROR_ERASE,
    ERROR_CHECK_ERASED,
    ERROR_PROG,
    ERROR_VERIFY,
    ERROR_ADDRESS,
    ERROR_NOTDONE,
    ERROR_FIRMWARE,
    ERROR_VENDOR,
    ERROR_USBR,
    ERROR_POR,
    ERROR_UNKNOWN,
    ERROR_STALLEDPKT,
};

std::string dfuStatusToString(DfuStatus status);
DfuStatus stringToDfuStatus(const std::string& str);

constexpr char kDfuStateAppIdle[] = "appIDLE";
constexpr char kDfuStateAppDetach[] = "appDETACH";
constexpr char kDfuStateDfuIdle[] = "dfuIDLE";
constexpr char kDfuStateDfuDnloadSync[] = "dfuDNLOAD-SYNC";
constexpr char kDfuStateDfuDnBusy[] = "dfuDNBUSY";
constexpr char kDfuStateDfuDnloadIdle[] = "dfuDNLOAD-IDLE";
constexpr char kDfuStateDfuManifestSync[] = "dfuMANIFEST-SYNC";
constexpr char kDfuStateDfuManifest[] = "dfuMANIFEST";
constexpr char kDfuStateDfuManifestWaitReset[] = "dfuMANIFEST-WAIT-RESET";
constexpr char kDfuStateDfuUploadIdle[] = "dfuUPLOAD-IDLE";
constexpr char kDfuStateDfuError[] = "dfuERROR";

enum DfuState : std::uint8_t
{
    APP_IDLE,
    APP_DETACH,
    DFU_IDLE,
    DFU_DNLOAD_SYNC,
    DFU_DNBUSY,
    DFU_DNLOAD_IDLE,
    DFU_MANIFEST_SYNC,
    DFU_MANIFEST,
    DFU_MANIFEST_WAIT_RST,
    DFU_UPLOAD_IDLE,
    DFU_ERROR,
};

std::string dfuStateToString(DfuState states);
DfuState stringToDfuState(const std::string& str);

enum DfuInterface : std::uint16_t
{
    DFU_IFF_DFU = 0x0001,
    DFU_IFF_VENDOR = 0x0100,
    DFU_IFF_PRODUCT = 0x0200,
    DFU_IFF_CONFIG = 0x0400,
    DFU_IFF_IFACE = 0x0800,
    DFU_IFF_ALT = 0x1000,
    DFU_IFF_DEVNUM = 0x2000,
    DFU_IFF_PATH = 0x4000,
};

/* This is based off of
DFU_GETSTATUS * * 1
unsigned byte bStatus * 3
unsigned byte bwPollTimeout * 1
unsigned byte bState * 1
unsigned byte iString */
struct DfuStatusData
{
    std::uint8_t bStatus{ERROR_UNKNOWN};
    std::uint32_t bwPollTimeout{};
    std::uint8_t bState{DFU_ERROR};
    std::uint8_t iString{};
};
}  // namespace dfu
