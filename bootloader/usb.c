// This file is part of the stenosaurus project.
//
// Copyright (C) 2013 Hesky Fisher <hesky.fisher@gmail.com>
//
// This library is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this library.  If not, see <http://www.gnu.org/licenses/>.
//
// This file defines the USB specific implementation of the bootloader.
//
// See the header file for interface documentation.
//
// It just so happens that Arm Cortex-M3 processors are little-endian and the
// USB descriptors need to be little endian too. This allows us to define our
// USB descriptors as more easily read structs, except for the HID description,
// which uses a more complex and variable encoding.

#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/hid.h>
#include <libopencm3/cm3/nvic.h>
#include "usb.h"
#include "../common/leds.h"
#include <libopencm3/cm3/scb.h>
#include <stdbool.h>

static const struct usb_device_descriptor device_descriptor = {
  // The size of this header in bytes, 18.
  .bLength = USB_DT_DEVICE_SIZE,
  // A value of 1 indicates that this is a device descriptor
  .bDescriptorType = USB_DT_DEVICE,
  // This device supports USB 2.0
  .bcdUSB = 0x0200,
  // Zero means that the class is defined at the interface level.
  .bDeviceClass = 0,
  // Subclass and Protocol are set to zero to indicate that they are defined at
  // the interface level.
  .bDeviceSubClass = 0,
  .bDeviceProtocol = 0,
  // Packet size for endpoint zero in bytes.
  .bMaxPacketSize0 = 64,
  // The id of the vendor (VID) who makes this device. This must be a VID 
  // assigned by the USB-IF. The VID/PID combo must be unique to a product. 
  // For now, we will use a VID reserved for prototypes and an arbitrary PID.
  .idVendor = 0x6666, // VID reserved for prototypes
  // Product ID within the Vendor ID space. The current PID is arbitrary since
  // we're using the prototype VID.
  .idProduct = 0x1,
  // Version number for the device. Set to 1.0.0 for now.
  .bcdDevice = 0x0100,
  // The index of the string in the string table that represents the name of
  // the manufacturer of this device.
  .iManufacturer = 1,
  // The index of the string in the string table that represents the name of the
  // product.
  .iProduct = 2,
  // The index of the string in the string table that represents the serial 
  // number of this item in string form. Zero means there isn't one.
  .iSerialNumber = 0,
  // The number of possible configurations this device has. This is one for 
  // most devices.
  .bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor hid_interface_endpoints[] = {
  {
    // The size of the endpoint descriptor in bytes: 7.
    .bLength = USB_DT_ENDPOINT_SIZE,
    // A value of 5 indicates that this describes an endpoint.
    .bDescriptorType = USB_DT_ENDPOINT,
    // Bit 7 indicates direction: 0 for OUT (to device) 1 for IN (to host).
    // Bits 6-4 must be set to 0.
    // Bits 3-0 indicate the endpoint number (zero is not allowed).
    // Here we define the IN side of endpoint 1.
    .bEndpointAddress = 0x81,
    // Bit 7-2 are only used in Isochronous mode, otherwise they should be 0.
    // Bit 1-0: Indicates the mode of this endpoint.
    // 00: Control
    // 01: Isochronous
    // 10: Bulk
    // 11: Interrupt
    // Here we're using interrupt.
    .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    // Maximum packet size.
    .wMaxPacketSize = 64,  // TODO: Seems high?
    // The frequency, in number of frames, that we're going to be sending data.
    // Here we're saying we're going to send data every frame (I think).
    .bInterval = 5,
  },
  {
    // The size of the endpoint descriptor in bytes: 7.
    .bLength = USB_DT_ENDPOINT_SIZE,
    // A value of 5 indicates that this describes an endpoint.
    .bDescriptorType = USB_DT_ENDPOINT,
    // Bit 7 indicates direction: 0 for OUT (to device) 1 for IN (to host).
    // Bits 6-4 must be set to 0.
    // Bits 3-0 indicate the endpoint number (zero is not allowed).
    // Here we define the OUT side of endpoint 1.
    .bEndpointAddress = 0x01,
    // Bit 7-2 are only used in Isochronous mode, otherwise they should be 0.
    // Bit 1-0: Indicates the mode of this endpoint.
    // 00: Control
    // 01: Isochronous
    // 10: Bulk
    // 11: Interrupt
    // Here we're using interrupt.
    .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    // Maximum packet size.
    .wMaxPacketSize = 64,  // TODO: Seems high?
    // The frequency, in number of frames, that we're going to be sending data.
    // Here we're saying we're going to send data every frame (I think).
    .bInterval = 7,
  }
};

// The data below is an HID report descriptor. The first byte in each item 
// indicates the number of bytes that follow in the lower two bits. The next two 
// bits indicate the type of the item. The remaining four bits indicate the tag.
// Words are stored in little endian.
// TODO: Understand this better. Maybe use the definition from rawhid?
static const uint8_t hid_report_descriptor[] = {
  // Usage Page = 0xFF00 (Vendor Defined Page 1)
  0x06, 0x00, 0xFF,
  // Usage (Vendor Usage 1)
  0x09, 0x01,
  // Collection (Application)
  0xA1, 0x01,
  //   Usage Minimum
  0x19, 0x01,
  //   Usage Maximum. 64 input usages total (0x01 to 0x40).
  0x29, 0x40,
  //   Logical Minimum (data bytes in the report may have minimum value = 0x00).
  0x15, 0x00,
  //   Logical Maximum (data bytes in the report may have 
  //     maximum value = 0x00FF = unsigned 255).
  // TODO: Can this be one byte?
  0x26, 0xFF, 0x00,
  //   Report Size: 8-bit field size
  0x75, 0x08,
  //   Report Count: Make sixty-four 8-bit fields (the next time the parser hits 
  //     an "Input", "Output", or "Feature" item).
  0x95, 0x40,
  //   Input (Data, Array, Abs): Instantiates input packet fields based on the 
  //     above report size, count, logical min/max, and usage.
  0x81, 0x00,
  //   Usage Minimum
  0x19, 0x01,
  //   Usage Maximum. 64 output usages total (0x01 to 0x40)
  0x29, 0x40,
  //   Output (Data, Array, Abs): Instantiates output packet fields. Uses same 
  //     report size and count as "Input" fields, since nothing new/different 
  //     was specified to the parser since the "Input" item.
  0x91, 0x00,
  // End Collection
  0xC0,
};

static const struct {
        struct usb_hid_descriptor hid_descriptor;
        struct {
                uint8_t bReportDescriptorType;
                uint16_t wDescriptorLength;
        } __attribute__((packed)) hid_report;
} __attribute__((packed)) hid_function = {
        .hid_descriptor = {
            // The size of this header in bytes: 9.
            .bLength = sizeof(hid_function),
            // The type of this descriptor. HID is indicated by the value 33.
            .bDescriptorType = USB_DT_HID,
            // The version of the HID spec used in binary coded  decimal. We are 
            // using version 1.11.
            .bcdHID = 0x0111,
            // Some HID devices, like keyboards, can specify different country
            // codes. A value of zero means not localized.
            .bCountryCode = 0,
            // The number of descriptors that follow. This must be at least one
            // since there should be at least a report descriptor.
            .bNumDescriptors = 1,
        },
        // The report descriptor.
        .hid_report = {
                // The type of descriptor. A value of 34 indicates a report.
                .bReportDescriptorType = USB_DT_REPORT,
                // The size of the descriptor defined above.
                .wDescriptorLength = sizeof(hid_report_descriptor),
        },
};

const struct usb_interface_descriptor hid_interface = {
  // The size of an interface descriptor: 9
  .bLength = USB_DT_INTERFACE_SIZE,
  // A value of 4 specifies that this describes and interface.
  .bDescriptorType = USB_DT_INTERFACE,
  // The number for this interface. Starts counting from 0.
  .bInterfaceNumber = 0,
  // The number for this alternate setting for this interface.
  .bAlternateSetting = 0,
  // The number of endpoints in this interface.
  .bNumEndpoints = 2,
  // The interface class for this interface is HID, defined by 3.
  .bInterfaceClass = USB_CLASS_HID,
  // The interface subclass for an HID device is used to indicate of this is
  // a mouse or keyboard that is boot mode capable (1) or not (0).
  .bInterfaceSubClass = 0, // Not a boot mode mouse or keyboard.
  .bInterfaceProtocol = 0, // Since subclass is zero then this must be too.
  // A string representing this interface. Zero means not provided.
  .iInterface = 0,
  // The header ends here.
  
  // A pointer to the beginning of the array of endpoints.
  .endpoint = hid_interface_endpoints,
  
  // Some class types require extra data in the interface descriptor. 
  // The libopencm3 usb library requires that we stuff that here.
  // Pointer to the buffer holding the extra data.
  .extra = &hid_function,
  // The length of the data at the above address.
  .extralen = sizeof(hid_function),
};

const struct usb_interface interfaces[] = {
  {
    .num_altsetting = 1,
    .altsetting = &hid_interface,
  }
};

static const struct usb_config_descriptor config_descriptor = {
  // The length of this header in bytes, 9.
  .bLength = USB_DT_CONFIGURATION_SIZE,
  // A value of 2 indicates that this is a configuration descriptor.
  .bDescriptorType = USB_DT_CONFIGURATION,
  // This should hold the total size of the configuration descriptor including
  // all sub interfaces. This is automatically filled in by the usb stack in
  // libopencm3.
  .wTotalLength = 0,
  // The number of interfaces in this configuration.
  .bNumInterfaces = 1,
  // The index of this configuration. Starts counting from 1.
  .bConfigurationValue = 1,
  // A string index describing this configration. Zero means not provided.
  .iConfiguration = 0,
  // Bit flags:
  // 7: Must be set to 1.
  // 6: This device is self powered.
  // 5: This device supports remote wakeup.
  // 4-0: Must be set to 0.
  .bmAttributes = 0b10000000,
  // The maximum amount of current that this device will draw in 2mA units. This
  // indicates 100mA.
  .bMaxPower = 50,
  // The header ends here.
  
  // A pointer to an array of interfaces.
  .interface = interfaces,
};

// The string table.
static const char *usb_strings[] = {
        "Open Steno Project",
        "Stenosaurus",
};

// This adds support for the additional control requests needed for the HID
// interface.
static int control_request_handler(usbd_device *dev, 
                                   struct usb_setup_data *req, 
                                   uint8_t **buf, 
                                   uint16_t *len,
                                   void (**complete)(usbd_device *, struct usb_setup_data *)) {
    (void)dev;
    (void)complete;

    // TODO: Check wIndex? I'm not sure if it needs to be zero or the interface index.
    // The request is:
    // - device to host
    // - A standard request
    // - recipient is an interface
    // Note: This function is only registered for 
    // USB_REQ_TYPE_IN | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE
    if (// - GetDescriptor
        (req->bRequest == USB_REQ_GET_DESCRIPTOR) &&
        // - High byte: Descriptor type is HID report (0x22)
        // - Low byte: Index 0
        (req->wValue == 0x2200)) {
        // Send the HID report descriptor.
        *buf = (uint8_t *)hid_report_descriptor;
        *len = sizeof(hid_report_descriptor);
        return USBD_REQ_HANDLED;
    }

    return USBD_REQ_NOTSUPP;
}

static bool (*packet_handler)(uint8_t*);
static uint8_t hid_buffer[64];

static void endpoint_callback(usbd_device *usbd_dev, uint8_t ep) {
    uint16_t bytes_read = usbd_ep_read_packet(usbd_dev, ep, hid_buffer, sizeof(hid_buffer));
    (void)bytes_read;
    // This function reads the packet and replaces it with the response buffer.
    bool reboot = packet_handler(hid_buffer);
    // If we don't send the whole buffer then hidapi doesn't read the report. Not sure why.
    usbd_ep_write_packet(usbd_dev, 0x81, hid_buffer, sizeof(hid_buffer));
    if (reboot) {
        led_toggle(2);
        for (volatile int i = 0; i < 800000; ++i);
        scb_reset_system();
    }
}

// The device is not configured for its function until the host chooses a 
// configuration even if the device only supports one configuration like this 
// one. This function sets up the real USB interface that we want to use. It 
// will be called again if the device is reset by the host so all setup needs to 
// happen here.
static void set_config_handler(usbd_device *dev, uint16_t wValue) {
    (void)dev;
    (void)wValue;

    // The address argument uses the MSB to indicate whether data is going in to
    // the host or out to the device (0 for out, 1 for in). 
    // Set up endpoint 1 for data going IN to the host.
    usbd_ep_setup(dev, 0x81, USB_ENDPOINT_ATTR_INTERRUPT, 64, NULL);
    // Set up endpoint 1 for data coming OUT from the host.
    usbd_ep_setup(dev, 0x01, USB_ENDPOINT_ATTR_INTERRUPT, 64, endpoint_callback);

    // The callback is registered for requests that are:
    // - device to host
    // - standard
    // - the recipient is an interface
    // It does this by applying the mask to bmRequestType and making sure it is
    // equal to the value.
    usbd_register_control_callback(dev,
                                   // The is the value.
                                   USB_REQ_TYPE_IN | USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
                                   // This is the mask.
                                   USB_REQ_TYPE_DIRECTION | USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
                                   control_request_handler);
}

// The buffer used for control requests. This needs to be big enough to hold
// any descriptor, the largest of which will be the configuration descriptor.
static uint8_t usbd_control_buffer[128];

// Structure holding all the info related to the usb device.
static usbd_device *usbd_dev;

// TODO: The driver should simply be chosen by the same variable as everything else.
void init_usb(bool (*handler)(uint8_t*)) {
    packet_handler = handler;
    usbd_dev = usbd_init(&stm32f103_usb_driver, &device_descriptor, 
                         &config_descriptor, usb_strings, sizeof(usb_strings), 
                         usbd_control_buffer, sizeof(usbd_control_buffer));
    usbd_register_set_config_callback(usbd_dev, set_config_handler);
    nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
    // Enable USB by raising up D+ via a 1.5K resistor.
    // This is done on the WaveShare board by removing the USB EN jumper and 
    // connecting PC0 to the right hand pin of the jumper port with a patch
    // wire. By setting PC0 to open drain it turns on an NFET which pulls 
    // up D+ via a 1.5K resistor.
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, 
                  GPIO0);
}

// This is the interrupt handler for low priority USB events. Implementing
// a function with this name makes it the function used for the interrupt.
// TODO: Handle the other USB interrupts.
void usb_lp_can_rx0_isr(void) {  
    usbd_poll(usbd_dev);
}
