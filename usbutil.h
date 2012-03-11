#ifndef USBUTIL_H
#define USBUTIL_H

#include <QLibrary>
#include "../d2xx/bin/ftd2xx.h"
#include "../d2xx/libusb/libusb/libusb.h"

/*
  FTDI Devices
    - Elektor SDR
    - SDR-IQ

  LibUSB Devices
    - Softrock
    - HPSDR

  HID Devices
    - FunCube Dongle
*/

/*
  Mac & Win USB utilities
*/

//Libusb 1.0 and 0.1 defines
//Support for LibUSB 1.0 for Mac testing
#define LIBUSB_VERSION1

#ifdef LIBUSB_VERSION1
    #define REQUEST_TYPE_VENDOR LIBUSB_REQUEST_TYPE_VENDOR
    #define RECIP_DEVICE LIBUSB_RECIPIENT_DEVICE
    #define ENDPOINT_IN LIBUSB_ENDPOINT_IN
    #define ENDPOINT_OUT LIBUSB_ENDPOINT_OUT
#else

#endif

class USBUtil
{
public:
    USBUtil();
    //Placeholders to make sure we have libraries available
    static bool LoadLibUsb() {return true;}
    static bool LoadFtdi() {return true;}

    static int FTDI_FindDeviceByName(QString deviceName);
    static int FTDI_FindDeviceBySerialNumber(QString serialNumber);
    //LibUSB
    static bool InitLibUsb();
#ifdef LIBUSB_VERSION1
    static bool OpenDevice(libusb_device *dev,libusb_device_handle **hDev);
    static bool CloseDevice(libusb_device_handle *hDev);
    static bool SetConfiguration(libusb_device_handle *hDev,int config);
    static bool Claim_interface(libusb_device_handle *hDev, int iface);
    static int ControlMsg(libusb_device_handle *hDev, uint8_t reqType, uint8_t req, uint16_t value, uint16_t index,
            unsigned char *data, uint16_t length, unsigned int timeout);

#endif
    static libusb_device_handle * LibUSB_FindAndOpenDevice(int PID, int VID, int multiple);

};

#endif // USBUTIL_H
