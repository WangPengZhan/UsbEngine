#include <string>
#include <filesystem>
#include <chrono>
#include <cstdlib>
#include <fstream>

#include <gtest/gtest.h>

#include "FastbootUsbDevice.h"
#include "Fastboot.h"
#include "UsbTransport.h"
#include "UsbClient.h"
#include "FastbootCommon.h"

namespace
{
bool hardwareTestsEnabled()
{
    const char* value = std::getenv("USBENGINE_RUN_HARDWARE_TESTS");
    return value != nullptr && std::string(value) == "1";
}
}  // namespace

class MockTransport : public fastboot::AbstractTransport
{
public:
    std::vector<std::string> replies;
    std::vector<std::vector<uint8_t>> writes;
    size_t next{};
    int readTimeout{};

    void setReadTimeout(int ms) override
    {
        readTimeout = ms;
    }

    int64_t read(std::vector<uint8_t>& buf) override
    {
        if (next == replies.size())
            return -1;
        const auto& reply = replies[next++];
        buf.assign(reply.begin(), reply.end());
        return buf.size();
    }

    int64_t write(const std::vector<uint8_t>& buf) override
    {
        writes.push_back(buf);
        return buf.size();
    }

    int close() override
    {
        return 0;
    }

    int reset() override
    {
        return 0;
    }
};

TEST(FastBootLogic, DownloadUsesDataHandshake)
{
    auto transport = std::make_unique<MockTransport>();
    auto* mock = transport.get();
    mock->replies = {"DATA00000004", "OKAY"};
    fastboot::FastBootHost host(std::move(transport));
    fastboot::FastbootResponse response;
    EXPECT_EQ(host.Download(std::vector<uint8_t>{1, 2, 3, 4}, response), fastboot::SUCCESS);
    ASSERT_EQ(mock->writes.size(), 2u);
    EXPECT_EQ(std::string(mock->writes[0].begin(), mock->writes[0].end()), "download:00000004");
    EXPECT_EQ(mock->writes[1], (std::vector<uint8_t>{1, 2, 3, 4}));
    EXPECT_EQ(response.result, fastboot::OKAY);
}

TEST(FastBootLogic, DownloadRejectsUnexpectedFinalResponse)
{
    auto transport = std::make_unique<MockTransport>();
    transport->replies = {"DATA00000002", "DATA00000002"};
    fastboot::FastBootHost host(std::move(transport));
    fastboot::FastbootResponse response;
    EXPECT_EQ(host.Download(std::vector<uint8_t>{1, 2}, response), fastboot::BAD_DEV_RESP);
}

TEST(FastBootLogic, FlashAllowsLongDeviceResponse)
{
    auto transport = std::make_unique<MockTransport>();
    auto* mock = transport.get();
    mock->replies = {"OKAY"};
    fastboot::FastBootHost host(std::move(transport));
    fastboot::FastbootResponse response;
    EXPECT_EQ(host.Flash("boot", response), fastboot::SUCCESS);
    EXPECT_EQ(mock->readTimeout, 120000);
}

TEST(FastBootLogic, FileDownloadUsesOneHandshake)
{
    auto path = std::filesystem::temp_directory_path() / "usbengine_fastboot_test.bin";
    {
        std::ofstream file(path, std::ios::binary);
        std::string data(70000, 'x');
        file.write(data.data(), data.size());
    }
    auto transport = std::make_unique<MockTransport>();
    auto* mock = transport.get();
    mock->replies = {"OKAY00020000", "DATA00011170", "OKAY"};
    fastboot::FastBootHost host(std::move(transport));
    fastboot::FastbootResponse response;
    std::vector<double> progress;
    EXPECT_EQ(host.Download(path.string(), response,
                            [&](double value) {
                                progress.push_back(value);
                            }),
              fastboot::SUCCESS);
    ASSERT_EQ(mock->writes.size(), 4u);
    EXPECT_EQ(std::string(mock->writes[1].begin(), mock->writes[1].end()), "download:00011170");
    EXPECT_EQ(mock->writes[2].size(), 65536u);
    EXPECT_EQ(mock->writes[3].size(), 4464u);
    ASSERT_EQ(progress.size(), 3u);
    EXPECT_EQ(progress[1], 99.0);
    EXPECT_EQ(progress[2], 100.0);
    std::filesystem::remove(path);
}

TEST(FastBootLogic, FileDownloadDoesNotReportCompletionOnFailure)
{
    auto path = std::filesystem::temp_directory_path() / "usbengine_fastboot_failed_test.bin";
    {
        std::ofstream file(path, std::ios::binary);
        file << "data";
    }
    auto transport = std::make_unique<MockTransport>();
    transport->replies = {"OKAY00020000", "DATA00000004", "FAILwrite error"};
    fastboot::FastBootHost host(std::move(transport));
    fastboot::FastbootResponse response;
    double lastProgress = 0;
    EXPECT_EQ(host.Download(path.string(), response,
                            [&](double value) {
                                lastProgress = value;
                            }),
              fastboot::DEVICE_FAIL);
    EXPECT_LT(lastProgress, 100.0);
    std::filesystem::remove(path);
}

constexpr uint16_t FASTBOOT_VID = 0x18D1;  // Google
constexpr uint16_t FASTBOOT_PID = 0x4EE0;

TEST(UsbDeviceTest, usb_init)
{
    if (!hardwareTestsEnabled())
    {
        GTEST_SKIP() << "Set USBENGINE_RUN_HARDWARE_TESTS=1 to run USB integration tests";
    }

    libusb::FastbootUsbDevice device(FASTBOOT_VID, FASTBOOT_PID);
    if (!device.valid())
    {
        std::cerr << "device init failed, errMsg: " << device.errorMessage() << std::endl;
        GTEST_SKIP() << "USB can't find";
    }
    std::cout << "find device!" << std::endl;
    std::cout << "VID: 0x" << std::hex << device.vid() << " PID: 0x" << device.pid() << std::dec << std::endl;
    std::cout << "EP OUT: 0x" << std::hex << (int)device.EpOut() << " EP IN: 0x" << (int)device.EpIn() << std::dec << std::endl;
    std::cout << "speed: " << device.speedInfo() << std::endl;
    std::cout << "max_packet_size: " << device.chunkSize() << " bytes" << std::endl;
    EXPECT_TRUE(device.valid());
    EXPECT_GT(device.chunkSize(), 0u);
    EXPECT_NE(device.EpIn(), 0u);
    EXPECT_NE(device.EpOut(), 0u);
}

TEST(FastBootTest, fastboot_version)
{
    if (!hardwareTestsEnabled())
    {
        GTEST_SKIP() << "Set USBENGINE_RUN_HARDWARE_TESTS=1 to run USB integration tests";
    }

    auto usbClient = std::make_unique<fastboot::USBClient>(FASTBOOT_VID, FASTBOOT_PID);
    auto transport = std::make_unique<fastboot::UsbTransport>();
    transport->setClient(std::move(usbClient));
    fastboot::FastBootHost host;
    host.setTransport(std::move(transport));
    fastboot::FastbootResponse result;
    std::vector<uint8_t> data(20, 20);
    fastboot::RetCode ret = host.GetVar("version", result);
    EXPECT_EQ(ret, fastboot::SUCCESS);
    EXPECT_EQ(result.result, fastboot::OKAY);
    EXPECT_EQ(result.message, "0.4");
}

TEST(FastBootTest, fastboot_download)
{
    if (!hardwareTestsEnabled())
    {
        GTEST_SKIP() << "Set USBENGINE_RUN_HARDWARE_TESTS=1 to run USB integration tests";
    }

    auto usbClient = std::make_unique<fastboot::USBClient>(FASTBOOT_VID, FASTBOOT_PID);
    auto transport = std::make_unique<fastboot::UsbTransport>();
    transport->setClient(std::move(usbClient));
    fastboot::FastBootHost host;
    host.setTransport(std::move(transport));
    fastboot::FastbootResponse result;
    std::vector<uint8_t> data(20, 20);
    fastboot::RetCode ret = host.Download(data, result);
    EXPECT_EQ(ret, fastboot::SUCCESS);
    EXPECT_EQ(result.result, fastboot::OKAY);
}

TEST(FastBootTest, fastboot_downloadFile)
{
    if (!hardwareTestsEnabled())
    {
        GTEST_SKIP() << "Set USBENGINE_RUN_HARDWARE_TESTS=1 to run USB integration tests";
    }

    std::string file_path = "u-boot.img";
    auto usbClient = std::make_unique<fastboot::USBClient>(FASTBOOT_VID, FASTBOOT_PID);
    auto transport = std::make_unique<fastboot::UsbTransport>();
    transport->setClient(std::move(usbClient));
    fastboot::FastBootHost host;
    host.setTransport(std::move(transport));
    fastboot::FastbootResponse response;
    auto start = std::chrono::steady_clock::now();
    fastboot::RetCode res = host.Download(file_path, response, {});
    auto end = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "[FastBoot] Download time: " << seconds << " s" << std::endl;
    auto fileSize = std::filesystem::file_size(file_path);
    double speed = (fileSize / 1'000'000.0) / seconds;
    std::cout << "[FastBoot] Speed: " << speed << " MB/s" << std::endl;
    EXPECT_EQ(res, fastboot::SUCCESS);
    EXPECT_EQ(response.result, fastboot::OKAY);
}
