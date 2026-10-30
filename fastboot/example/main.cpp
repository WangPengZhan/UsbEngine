#include <iostream>

#include "FastbootUsbDevice.h"
#include "LibusbHelper.h"

int main(int argc, char** argv)
{
    auto devices = libusb::findFastbootDevices();
    if (devices.empty())
    {
        std::cerr << "No Fastboot device found" << std::endl;
        return 1;
    }

    uint16_t vid = devices.front().first;
    uint16_t pid = devices.front().second;

    std::cout << "Found DFU device: VID=0x" << std::hex << vid << " PID=0x" << pid << std::dec << std::endl;

    libusb::FastbootUsbDevice device(vid, pid);

    std::cout << "=== DFU Device Info ===" << std::endl;
    std::cout << "Name: " << device.name() << std::endl;
    std::cout << "VID: 0x" << std::hex << device.vid() << " PID: 0x" << device.pid() << std::dec << std::endl;
    std::cout << "Speed: " << device.speedInfo() << std::endl;
    std::cout << "Chunk size: " << device.chunkSize() << std::endl;

    std::cout << "EP IN: 0x" << std::hex << static_cast<int>(device.EpIn()) << std::endl;
    std::cout << "EP OUT: 0x" << std::hex << static_cast<int>(device.EpOut()) << std::dec << std::endl;

    std::cout << "=== DFU Initialization Complete ===" << std::endl;

    return 0;
}
