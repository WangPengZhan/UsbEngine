#include "DfuCommon.h"

namespace dfu
{
std::string dfuStatusToString(DfuStatus status)
{
    switch (status)
    {
    case OK:
        return kDfuStatusOk;
    case ERROR_TARGET:
        return kDfuStatusErrorTarget;
    case ERROR_FILE:
        return kDfuStatusErrorFile;
    case ERROR_WRITE:
        return kDfuStatusErrorWrite;
    case ERROR_ERASE:
        return kDfuStatusErrorErase;
    case ERROR_CHECK_ERASED:
        return kDfuStatusErrorCheckErased;
    case ERROR_PROG:
        return kDfuStatusErrorProg;
    case ERROR_VERIFY:
        return kDfuStatusErrorVerify;
    case ERROR_ADDRESS:
        return kDfuStatusErrorAddress;
    case ERROR_NOTDONE:
        return kDfuStatusErrorNotDone;
    case ERROR_FIRMWARE:
        return kDfuStatusErrorFirmware;
    case ERROR_VENDOR:
        return kDfuStatusErrorVendor;
    case ERROR_USBR:
        return kDfuStatusErrorUsbr;
    case ERROR_POR:
        return kDfuStatusErrorPor;
    case ERROR_UNKNOWN:
        return kDfuStatusErrorUnknown;
    case ERROR_STALLEDPKT:
        return kDfuStatusErrorStalledPkt;
    default:
        return "Invalid DFU status";
    }
}

DfuStatus stringToDfuStatus(const std::string& str)
{
    if (str == kDfuStatusOk)
        return OK;
    if (str == kDfuStatusErrorTarget)
        return ERROR_TARGET;
    if (str == kDfuStatusErrorFile)
        return ERROR_FILE;
    if (str == kDfuStatusErrorWrite)
        return ERROR_WRITE;
    if (str == kDfuStatusErrorErase)
        return ERROR_ERASE;
    if (str == kDfuStatusErrorCheckErased)
        return ERROR_CHECK_ERASED;
    if (str == kDfuStatusErrorProg)
        return ERROR_PROG;
    if (str == kDfuStatusErrorVerify)
        return ERROR_VERIFY;
    if (str == kDfuStatusErrorAddress)
        return ERROR_ADDRESS;
    if (str == kDfuStatusErrorNotDone)
        return ERROR_NOTDONE;
    if (str == kDfuStatusErrorFirmware)
        return ERROR_FIRMWARE;
    if (str == kDfuStatusErrorVendor)
        return ERROR_VENDOR;
    if (str == kDfuStatusErrorUsbr)
        return ERROR_USBR;
    if (str == kDfuStatusErrorPor)
        return ERROR_POR;
    if (str == kDfuStatusErrorUnknown)
        return ERROR_UNKNOWN;
    if (str == kDfuStatusErrorStalledPkt)
        return ERROR_STALLEDPKT;
    return ERROR_UNKNOWN;
}

std::string dfuStateToString(DfuState state)
{
    switch (state)
    {
    case APP_IDLE:
        return kDfuStateAppIdle;
    case APP_DETACH:
        return kDfuStateAppDetach;
    case DFU_IDLE:
        return kDfuStateDfuIdle;
    case DFU_DNLOAD_SYNC:
        return kDfuStateDfuDnloadSync;
    case DFU_DNBUSY:
        return kDfuStateDfuDnBusy;
    case DFU_DNLOAD_IDLE:
        return kDfuStateDfuDnloadIdle;
    case DFU_MANIFEST_SYNC:
        return kDfuStateDfuManifestSync;
    case DFU_MANIFEST:
        return kDfuStateDfuManifest;
    case DFU_MANIFEST_WAIT_RST:
        return kDfuStateDfuManifestWaitReset;
    case DFU_UPLOAD_IDLE:
        return kDfuStateDfuUploadIdle;
    case DFU_ERROR:
        return kDfuStateDfuError;
    default:
        return "Invalid DFU state";
    }
}

DfuState stringToDfuState(const std::string& s)
{
    if (s == kDfuStateAppIdle)
        return APP_IDLE;
    if (s == kDfuStateAppDetach)
        return APP_DETACH;
    if (s == kDfuStateDfuIdle)
        return DFU_IDLE;
    if (s == kDfuStateDfuDnloadSync)
        return DFU_DNLOAD_SYNC;
    if (s == kDfuStateDfuDnBusy)
        return DFU_DNBUSY;
    if (s == kDfuStateDfuDnloadIdle)
        return DFU_DNLOAD_IDLE;
    if (s == kDfuStateDfuManifestSync)
        return DFU_MANIFEST_SYNC;
    if (s == kDfuStateDfuManifest)
        return DFU_MANIFEST;
    if (s == kDfuStateDfuManifestWaitReset)
        return DFU_MANIFEST_WAIT_RST;
    if (s == kDfuStateDfuUploadIdle)
        return DFU_UPLOAD_IDLE;
    if (s == kDfuStateDfuError)
        return DFU_ERROR;
    return DFU_ERROR;  // fallback
}
}  // namespace dfu
