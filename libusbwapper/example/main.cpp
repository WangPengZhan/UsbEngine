#include <iostream>

#include "DfuUsbDevice.h"
#include "LibusbHelper.h"

int main(int argc, char** argv)
{
    auto devices = libusb::findDFUDevices();
    if (devices.empty())
    {
        std::cerr << "No DFU device found" << std::endl;
        return 1;
    }

    uint16_t vid = devices.front().first;
    uint16_t pid = devices.front().second;

    std::cout << "Found DFU device: VID=0x" << std::hex << vid << " PID=0x" << pid << std::dec << std::endl;

    libusb::DfuUsbDevice device(vid, pid);

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

    return 0;
}
