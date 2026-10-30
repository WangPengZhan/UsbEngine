#include <string>
#include <filesystem>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iterator>

#include <gtest/gtest.h>

#include "LibusbHelper.h"
#include "DfuUsbDevice.h"
#include "DfuHost.h"
#include "DfuFile.h"
#include "DfuSe.h"

namespace
{
bool hardwareTestsEnabled()
{
    const char* value = std::getenv("USBENGINE_RUN_HARDWARE_TESTS");
    return value != nullptr && std::string(value) == "1";
}
}  // namespace

TEST(DfuFileLogic, MemoryLayoutMultipleRegions)
{
    auto segments = dfu::parseMemoryLayout("@Flash/0x08000000/02*016Ka,01*032Kg/0x20000000/01*001Kb");
    ASSERT_EQ(segments.size(), 3u);
    auto it = segments.begin();
    EXPECT_EQ(it->start, 0x08000000u);
    EXPECT_EQ(it->end, 0x08007fffu);
    EXPECT_EQ(it->memtype, 1u);
    ++it;
    EXPECT_EQ(it->start, 0x08008000u);
    EXPECT_EQ(it->memtype, 7u);
    ++it;
    EXPECT_EQ(it->start, 0x20000000u);
    EXPECT_TRUE(dfu::parseMemoryLayout("@Flash/garbage/02*016Ka").empty());
    auto option = dfu::parseMemoryLayout("@Option Bytes /0x1FFFF800/01*016 g");
    ASSERT_EQ(option.size(), 1u);
    EXPECT_EQ(option.front().memtype, 7u);
}

TEST(DfuFileLogic, SuffixRoundTripAndCorruption)
{
    auto path = std::filesystem::temp_directory_path() / "usbengine_dfu_test.bin";
    {
        std::ofstream file(path, std::ios::binary);
        file << "firmware";
    }
    dfu::DfuSuffix suffix{};
    suffix.bcdDFU = 0x011a;
    ASSERT_TRUE(dfu::DfuFile::addDfuSuffix(path.string(), suffix));
    EXPECT_FALSE(dfu::DfuFile::addDfuSuffix(path.string(), suffix));
    std::ifstream encoded(path, std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(encoded)), std::istreambuf_iterator<char>());
    encoded.close();
    ASSERT_GE(bytes.size(), 16u);
    EXPECT_EQ(std::string(bytes.end() - 8, bytes.end() - 5), "UFD");
    uint32_t crc = 0xffffffff;
    for (size_t i = 0; i < bytes.size() - 4; ++i)
    {
        crc ^= bytes[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320u : 0u);
    }
    uint32_t stored = static_cast<uint32_t>(bytes[bytes.size() - 4]) | (static_cast<uint32_t>(bytes[bytes.size() - 3]) << 8) |
                      (static_cast<uint32_t>(bytes[bytes.size() - 2]) << 16) | (static_cast<uint32_t>(bytes[bytes.size() - 1]) << 24);
    EXPECT_EQ(stored, crc);
    EXPECT_TRUE(dfu::DfuFile::parseDfuFile(path.string()).isValid());
    {
        std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
        file.put('X');
    }
    EXPECT_FALSE(dfu::DfuFile::parseDfuFile(path.string()).isValid());
    auto corrupt = dfu::DfuFile::parseDfuFile(path.string());
    EXPECT_EQ(std::string(corrupt.suffix().ucDfuSignature, 3), "UFD");
    std::filesystem::remove(path);
}

TEST(DFUDevice, device_find)
{
    if (!hardwareTestsEnabled())
    {
        GTEST_SKIP() << "Set USBENGINE_RUN_HARDWARE_TESTS=1 to run USB integration tests";
    }

    auto device1 = libusb::findDFUDevices();
    auto device2 = libusb::findFastbootDevices();
    EXPECT_EQ(device1.empty() != device2.empty(), true);
}

TEST(DFUDevice, device_init)
{
    if (!hardwareTestsEnabled())
    {
        GTEST_SKIP() << "Set USBENGINE_RUN_HARDWARE_TESTS=1 to run USB integration tests";
    }

    auto devices = libusb::findDFUDevices();
    ASSERT_FALSE(devices.empty()) << "No DFU devices found";
    uint16_t vid = devices.front().first;
    uint16_t pid = devices.front().second;
    std::cout << "Found DFU device: VID=0x" << std::hex << vid << " PID=0x" << pid << std::dec << std::endl;
    libusb::DfuUsbDevice device(vid, pid);
    ASSERT_TRUE(device.valid()) << "Invalid DFU device: " << device.errorMessage();
    std::cout << "=== DFU Device Info ===" << std::endl;
    std::cout << "Name: " << device.name() << std::endl;
    std::cout << "VID: 0x" << std::hex << device.vid() << " PID: 0x" << device.pid() << std::dec << std::endl;
    std::cout << "Speed: " << device.speedInfo() << std::endl;
    std::cout << "Chunk size: " << device.chunkSize() << std::endl;
    std::cout << "bcdDevice: 0x" << std::hex << device.bcdDevice() << std::dec << std::endl;
    std::cout << "Configuration: " << device.configuration() << std::endl;
    std::cout << "Interface: " << device.interface() << std::endl;
    std::cout << "AltSetting: " << device.altsetting() << std::endl;
    std::cout << "Service Name: " << device.serviceName() << std::endl;
    std::cout << "Quirks: 0x" << std::hex << device.quirks() << std::dec << std::endl;
    std::cout << "=== DFU Initialization Complete ===" << std::endl;
}

TEST(DFUTest, dfu_download)
{
    if (!hardwareTestsEnabled())
    {
        GTEST_SKIP() << "Set USBENGINE_RUN_HARDWARE_TESTS=1 to run USB integration tests";
    }

    auto devices = libusb::findDFUDevices();
    ASSERT_FALSE(devices.empty()) << "No DFU devices found";
    uint16_t vid = devices.front().first;
    uint16_t pid = devices.front().second;
    std::cout << "Found DFU device: VID=0x" << std::hex << vid << " PID=0x" << pid << std::dec << std::endl;
    dfu::DfuHost host(vid, pid);
    std::vector<uint8_t> data(20, 20);
    auto st = host.getStatus();
    if (st.bState == dfu::DFU_ERROR)
    {
        bool ret = host.clearStatus();
        EXPECT_EQ(ret, true);
    }
    bool ret = host.download(data);
    EXPECT_EQ(ret, true);
    ret = host.downloadFinished();
    EXPECT_EQ(ret, true);
    host.waitSyncState();
}

TEST(DFUTest, dfu_downloadFile)
{
    if (!hardwareTestsEnabled())
    {
        GTEST_SKIP() << "Set USBENGINE_RUN_HARDWARE_TESTS=1 to run USB integration tests";
    }

    std::string file_path = "otsl.img";
    auto devices = libusb::findDFUDevices();
    ASSERT_FALSE(devices.empty()) << "No DFU devices found";
    uint16_t vid = devices.front().first;
    uint16_t pid = devices.front().second;
    std::cout << "Found DFU device: VID=0x" << std::hex << vid << " PID=0x" << pid << std::dec << std::endl;
    dfu::DfuHost host(vid, pid);
    auto start = std::chrono::steady_clock::now();
    bool ret = host.download(file_path);
    auto end = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "[DFU] Download time: " << seconds << " s" << std::endl;
    auto fileSize = std::filesystem::file_size(file_path);
    double speed = (fileSize / 1'000'000.0) / seconds;
    std::cout << "[DFU] Speed: " << speed << " MB/s" << std::endl;
    EXPECT_EQ(ret, true);
}

TEST(DFUTest, dfu_upload)
{
    if (!hardwareTestsEnabled())
    {
        GTEST_SKIP() << "Set USBENGINE_RUN_HARDWARE_TESTS=1 to run USB integration tests";
    }

    auto devices = libusb::findDFUDevices();
    ASSERT_FALSE(devices.empty()) << "No DFU devices found";
    uint16_t vid = devices.front().first;
    uint16_t pid = devices.front().second;
    std::cout << "Found DFU device: VID=0x" << std::hex << vid << " PID=0x" << pid << std::dec << std::endl;
    dfu::DfuHost host(vid, pid);
    std::vector<uint8_t> data;
    auto st = host.getStatus();
    if (st.bState == dfu::DFU_ERROR)
    {
        bool ret = host.clearStatus();
        EXPECT_EQ(ret, true);
    }
    bool ret = host.upload(data);
    EXPECT_EQ(ret, true);
}

TEST(DFUTest, dfu_uploadFile)
{
    if (!hardwareTestsEnabled())
    {
        GTEST_SKIP() << "Set USBENGINE_RUN_HARDWARE_TESTS=1 to run USB integration tests";
    }

    auto devices = libusb::findDFUDevices();
    ASSERT_FALSE(devices.empty()) << "No DFU devices found";
    uint16_t vid = devices.front().first;
    uint16_t pid = devices.front().second;
    std::cout << "Found DFU device: VID=0x" << std::hex << vid << " PID=0x" << pid << std::dec << std::endl;
    dfu::DfuHost host(vid, pid);
    auto st = host.getStatus();
    if (st.bState == dfu::DFU_ERROR)
    {
        bool ret = host.clearStatus();
        EXPECT_EQ(ret, true);
    }
    std::string filename = "test.txt";
    auto start = std::chrono::steady_clock::now();
    bool ret = host.upload(filename);
    auto end = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "[DFU] Upload time: " << seconds << " s" << std::endl;
    auto fileSize = std::filesystem::file_size(filename);
    double speed = (fileSize / 1'000'000.0) / seconds;
    std::cout << "[DFU] Speed: " << speed << " MB/s" << std::endl;
    EXPECT_EQ(ret, true);
}

TEST(DFUTest, dfu_getStatus)
{
    if (!hardwareTestsEnabled())
    {
        GTEST_SKIP() << "Set USBENGINE_RUN_HARDWARE_TESTS=1 to run USB integration tests";
    }

    auto devices = libusb::findDFUDevices();
    ASSERT_FALSE(devices.empty()) << "No DFU devices found";
    uint16_t vid = devices.front().first;
    uint16_t pid = devices.front().second;
    std::cout << "Found DFU device: VID=0x" << std::hex << vid << " PID=0x" << pid << std::dec << std::endl;
    dfu::DfuHost host(vid, pid);
    auto st = host.getStatus();
    EXPECT_EQ(st.bState, dfu::DFU_IDLE);
}
