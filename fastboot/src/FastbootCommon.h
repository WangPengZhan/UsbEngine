#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace fastboot
{
constexpr int kResultMaxLength = 64;

enum FastbootResult
{
    UNKOWN,
    OKAY,
    FAIL,
    INFO,
    DATA,
    TEXT,
};

constexpr char OKAY_STR[] = "OKAY";
constexpr char FAIL_STR[] = "FAIL";
constexpr char INFO_STR[] = "INFO";
constexpr char DATA_STR[] = "DATA";
constexpr char TEXT_STR[] = "TEXT";

std::string resultToString(FastbootResult result);
FastbootResult stringToResult(const std::string& result);

bool isValidResult(const std::string& result);

struct FastbootResponse
{
    FastbootResult result{UNKOWN};
    std::string message;
    std::uint32_t dsize{};
    std::string toString() const;
    static FastbootResponse parse(const std::string& content);
};

constexpr int kCMDMaxLength = 4096;  // standard
constexpr char CMD_GETVAR[] = "getvar";
constexpr char CMD_DOWNLOAD[] = "download";
constexpr char CMD_VERIFY[] = "verify";
constexpr char CMD_FLASH[] = "flash";
constexpr char CMD_ERASE[] = "erase";
constexpr char CMD_BOOT[] = "boot";
constexpr char CMD_CONTINUE[] = "continue";
constexpr char CMD_REBOOT[] = "reboot";
constexpr char CMD_REBOOT_BOOTLOADER[] = "reboot-bootloader";
constexpr char CMD_POWEROFF[] = "powerdown";

constexpr char CMD_UCMD[] = "ucmd";  // Synchronous bootloader command
constexpr char CMD_ACMD[] = "acmd";  // Asynchronous bootloader command

// extend
constexpr char CMD_DOWNLOAD_SPARSE[] = "download-sparse";  // Android Sparse image
constexpr char CMD_GET_STAGED[] = "get_staged";
constexpr char CMD_STAGE[] = "stage";
constexpr char CMD_SET_ACTIVE[] = "set_active";
constexpr char CMD_GET_STAGED_SIZE[] = "get_staged_size";

struct CmdInfo
{
    std::string cmdName;
    std::vector<std::string> args;
    std::string toString() const;
};

}  // namespace fastboot
