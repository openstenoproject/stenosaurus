// This file is part of the stenosaurus project.
//
// Copyright (C) 2013 Hesky Fisher <hesky.fisher@gmail.com>
//
// This library is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this library.  If not, see <http://www.gnu.org/licenses/>.
//
// This file implements the USB interface to the Stenosaurus. See the header
// file for interface documentation to this code.

#include "usb.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/usb/hid.h>
#include <libopencm3/usb/usbd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "../common/leds.h"


enum {
    INTERFACE_RAW_HID = 0,
    // The next two must be consecutive since they are used in an Interface
    // Assication below. If the order is changed then the IAD must be changed as
    // well
    INTERFACE_CDC_COMM = 1,
    INTERFACE_CDC_DATA = 2,
    INTERFACE_KEYBOARD_HID = 3,
    INTERFACE_COUNT = 4,
};

enum {
    ENDPOINT_RAW_HID_IN = 0x81,
    ENDPOINT_RAW_HID_OUT = 0x01,
    ENDPOINT_CDC_COMM_IN = 0x83,
    ENDPOINT_CDC_DATA_IN = 0x82,
    ENDPOINT_CDC_DATA_OUT = 0x02,
    ENDPOINT_KEYBOARD_HID_IN = 0x84,
};

enum {
    HID_GET_REPORT = 1,
    HID_GET_IDLE = 2,
    HID_GET_PROTOCOL = 3,
    HID_SET_REPORT = 9,
    HID_SET_IDLE = 10,
    HID_SET_PROTOCOL = 11,
};

// It just so happens that Arm Cortex-M3 processors are little-endian and the
// USB descriptors need to be little endian too. This allows us to define our
// USB descriptors as more easily read structs (compared to byte arrays), except
// for the HID description, which uses a more complex and variable length
// encoding.

static const struct usb_device_descriptor device_descriptor = {
    // The size of this descriptor in bytes, 18.
    .bLength = USB_DT_DEVICE_SIZE,
    // A value of 1 indicates that this is a device descriptor.
    .bDescriptorType = USB_DT_DEVICE,
    // This device supports USB 2.0
    .bcdUSB = 0x0200,
    // When cereating a multi-function device with more than one interface per
    // logical function (as we are doing with the CDC interfaces below to create
    // a virtual serial device) one  must use Interface Association Descriptors
    // and the next three values must be set to the exact values specified. The
    // values have assigned meanings, which are mentioned in the comments, but
    // since they must be used when using IADs that makes their given
    // definitions meaningless. See
    // http://www.usb.org/developers/docs/InterfaceAssociationDescriptor_ecn.pdf
    // and http://www.usb.org/developers/whitepapers/iadclasscode_r10.pdf
    .bDeviceClass = 0xEF, // Miscellaneous Device.
    .bDeviceSubClass = 2, // Common Class
    .bDeviceProtocol = 1, // Interface Association
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
    // The index of the string in the string table that represents the name of
    // the product.
    .iProduct = 2,
    // The index of the string in the string table that represents the serial
    // number of this item in string form. Zero means there isn't one.
    .iSerialNumber = 0,
    // The number of possible configurations this device has. This is one for
    // most devices.
    .bNumConfigurations = 1,
};

static const struct usb_endpoint_descriptor raw_hid_interface_endpoints[] = {
    {
        // The size of the endpoint descriptor in bytes: 7.
        .bLength = USB_DT_ENDPOINT_SIZE,
        // A value of 5 indicates that this describes an endpoint.
        .bDescriptorType = USB_DT_ENDPOINT,
        // Bit 7 indicates direction: 0 for OUT (to device) 1 for IN (to host).
        // Bits 6-4 must be set to 0.
        // Bits 3-0 indicate the endpoint number (zero is not allowed).
        // Here we define the IN side of endpoint 1.
        .bEndpointAddress = ENDPOINT_RAW_HID_IN,
        // Bit 7-2 are only used in Isochronous mode, otherwise they should be
        // 0.
        // Bit 1-0: Indicates the mode of this endpoint.
        // 00: Control
        // 01: Isochronous
        // 10: Bulk
        // 11: Interrupt
        // Here we're using interrupt.
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        // Maximum packet size.
        .wMaxPacketSize = 64,
        // The frequency, in number of frames, that we're going to be sending
        // data. Here we're saying we're going to send data every millisecond.
        .bInterval = 10,
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
        .bEndpointAddress = ENDPOINT_RAW_HID_OUT,
        // Bit 7-2 are only used in Isochronous mode, otherwise they should be
        // 0.
        // Bit 1-0: Indicates the mode of this endpoint.
        // 00: Control
        // 01: Isochronous
        // 10: Bulk
        // 11: Interrupt
        // Here we're using interrupt.
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        // Maximum packet size.
        .wMaxPacketSize = 64,
        // The frequency, in number of frames, that we're going to be sending
        // data. Here we're saying we're going to send data every millisecond.
        .bInterval = 10,
    }
};

// The data below is an HID report descriptor. The first byte in each item
// indicates the number of bytes that follow in the lower two bits. The next two
// bits indicate the type of the item. The remaining four bits indicate the tag.
// Words are stored in little endian.
static const uint8_t raw_hid_report_descriptor[] = {
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
    //   Logical Minimum (data bytes in the report may have minimum value =
    //   0x00).
    0x15, 0x00,
    //   Logical Maximum (data bytes in the report may have
    //     maximum value = 0x00FF = unsigned 255).
    // TODO: Can this be one byte?
    0x26, 0xFF, 0x00,
    //   Report Size: 8-bit field size
    0x75, 0x08,
    //   Report Count: Make sixty-four 8-bit fields (the next time the parser
    //     hits an "Input", "Output", or "Feature" item).
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
} __attribute__((packed)) raw_hid_function = {
    .hid_descriptor = {
        // The size of this header in bytes: 9.
        .bLength = sizeof(raw_hid_function),
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
        .wDescriptorLength = sizeof(raw_hid_report_descriptor),
    },
};

const struct usb_interface_descriptor raw_hid_interface = {
    // The size of an interface descriptor: 9
    .bLength = USB_DT_INTERFACE_SIZE,
    // A value of 4 specifies that this describes and interface.
    .bDescriptorType = USB_DT_INTERFACE,
    // The number for this interface. Starts counting from 0.
    .bInterfaceNumber = INTERFACE_RAW_HID,
    // The number for this alternate setting for this interface.
    .bAlternateSetting = 0,
    // The number of endpoints in this interface.
    .bNumEndpoints = 2,
    // The interface class for this interface is HID, defined by 3.
    .bInterfaceClass = USB_CLASS_HID,
    // The interface subclass for an HID device is used to indicate of this is a
    // mouse or keyboard that is boot mode capable (1) or not (0).
    .bInterfaceSubClass = 0, // Not a boot mode mouse or keyboard.
    .bInterfaceProtocol = 0, // Since subclass is zero then this must be too.
    // A string representing this interface. Zero means not provided.
    .iInterface = 0,
    // The header ends here.

    // A pointer to the beginning of the array of endpoints.
    .endpoint = raw_hid_interface_endpoints,

    // Some class types require extra data in the interface descriptor.
    // The libopencm3 usb library requires that we stuff that here.
    // Pointer to the buffer holding the extra data.
    .extra = &raw_hid_function,
    // The length of the data at the above address.
    .extralen = sizeof(raw_hid_function),
};

static const struct usb_endpoint_descriptor cdc_comm_endpoints[] = {
    {
        // The size of the endpoint descriptor in bytes: 7.
        .bLength = USB_DT_ENDPOINT_SIZE,
        // A value of 5 indicates that this describes an endpoint.
        .bDescriptorType = USB_DT_ENDPOINT,
        // Bit 7 indicates direction: 0 for OUT (to device) 1 for IN (to host).
        // Bits 6-4 must be set to 0.
        // Bits 3-0 indicate the endpoint number (zero is not allowed).
        // Here we define the IN side of endpoint 3.
        .bEndpointAddress = ENDPOINT_CDC_COMM_IN,
        // Bit 7-2 are only used in Isochronous mode, otherwise they should be
        // 0.
        // Bit 1-0: Indicates the mode of this endpoint.
        // 00: Control
        // 01: Isochronous
        // 10: Bulk
        // 11: Interrupt
        // Here we're using interrupt.
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        // Maximum packet size.
        .wMaxPacketSize = 16,
        // The frequency, in number of frames, that we're going to be sending
        // data. Here we're saying we're going to send data every 255
        // miliseconds. Since this endpoint is completely unused we use the
        // largest interval possible.
        .bInterval = 255,
    }
};

static const struct usb_endpoint_descriptor cdc_data_endpoints[] = {
    {
        // The size of the endpoint descriptor in bytes: 7.
        .bLength = USB_DT_ENDPOINT_SIZE,
        // A value of 5 indicates that this describes an endpoint.
        .bDescriptorType = USB_DT_ENDPOINT,
        // Bit 7 indicates direction: 0 for OUT (to device) 1 for IN (to host).
        // Bits 6-4 must be set to 0.
        // Bits 3-0 indicate the endpoint number (zero is not allowed).
        // Here we define the OUT side of endpoint 2.
        .bEndpointAddress = ENDPOINT_CDC_DATA_OUT,
        // Bit 7-2 are only used in Isochronous mode, otherwise they should be
        // 0.
        // Bit 1-0: Indicates the mode of this endpoint.
        // 00: Control
        // 01: Isochronous
        // 10: Bulk
        // 11: Interrupt
        // Here we're using Bulk.
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        // Maximum packet size.
        .wMaxPacketSize = 64,
        // This field is ignored for bulk endpoints.
        .bInterval = 1,
    },
    {
        // The size of the endpoint descriptor in bytes: 7.
        .bLength = USB_DT_ENDPOINT_SIZE,
        // A value of 5 indicates that this describes an endpoint.
        .bDescriptorType = USB_DT_ENDPOINT,
        // Bit 7 indicates direction: 0 for OUT (to device) 1 for IN (to host).
        // Bits 6-4 must be set to 0.
        // Bits 3-0 indicate the endpoint number (zero is not allowed).
        // Here we define the IN side of endpoint 2.
        .bEndpointAddress = ENDPOINT_CDC_DATA_IN,
        // Bit 7-2 are only used in Isochronous mode, otherwise they should be
        // 0.
        // Bit 1-0: Indicates the mode of this endpoint.
        // 00: Control
        // 01: Isochronous
        // 10: Bulk
        // 11: Interrupt
        // Here we're using Bulk.
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        // Maximum packet size.
        .wMaxPacketSize = 64,
        // This field is ignored for bulk endpoints.
        .bInterval = 1,
    }
};

static const struct {
    struct usb_cdc_header_descriptor header;
    struct usb_cdc_call_management_descriptor call_mgmt;
    struct usb_cdc_acm_descriptor acm;
    struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
    .header = {
        // The size of the CDC header descriptor: 5.
        .bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
        // Class specific interface. i.e. the interface constant (4) with the
        // class bit set making it 0x24 or 36.
        .bDescriptorType = CS_INTERFACE,
        // Setting this field to zero marks this as the beginning of a set of
        // descriptors describing this CDC device.
        .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
        // This device complies with version 1.1 of the USB CDC specification.
        .bcdCDC = 0x0110,
    },
    .call_mgmt = {
        // The length of this descriptor: 5.
        .bFunctionLength = sizeof(struct usb_cdc_call_management_descriptor),
        // Class specific interface. i.e. the interface constant (4) with the
        // class bit set making it 0x24 or 36.
        .bDescriptorType = CS_INTERFACE,
        // This descriptor defines call management for this communications
        // device.
        .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
        // A value of zero indicates that the device does not handle call
        // management.
        .bmCapabilities = 0,
        // This is the index of the data class interface.
        .bDataInterface = INTERFACE_CDC_DATA,
    },
    .acm = {
        // The size of this descriptor: 4.
        .bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
        // Class specific interface. i.e. the interface constant (4) with the
        // class bit set making it 0x24 or 36.
        .bDescriptorType = CS_INTERFACE,
        // This descriptor defines which commands this device supports.
        .bDescriptorSubtype = USB_CDC_TYPE_ACM,
        // Zero means that none of the standard commands are supported.
        .bmCapabilities = 0,
    },
    .cdc_union = {
        // The length of this descriptor: 5.
        .bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
        // Class specific interface. i.e. the interface constant (4) with the
        // class bit set making it 0x24 or 36.
        .bDescriptorType = CS_INTERFACE,
        // To quote the spec: "The Union functional descriptor describes the
        // relationship between a group of interfaces that can be considered to
        // form a functional unit. [...] One of the interfaces in the group is
        // designated as a master or controlling interface for the group, and
        // certain class- specific messages can be sent to this interface to act
        // upon the group as a whole. Similarly, notifications for the entire
        // group can be sent from this interface but apply to the entire group
        // of interfaces."
        .bDescriptorSubtype = USB_CDC_TYPE_UNION,
        // The index of the control interface.
        .bControlInterface = 1,
        // The index of the subordinate interface.
        .bSubordinateInterface0 = 2,
    }
};

static const struct usb_interface_descriptor cdc_comm_interface = {
    // The size of an interface descriptor: 9
    .bLength = USB_DT_INTERFACE_SIZE,
    // A value of 4 specifies that this describes and interface.
    .bDescriptorType = USB_DT_INTERFACE,
    // The number for this interface. Starts counting from 0.
    .bInterfaceNumber = INTERFACE_CDC_COMM,
    // The number for this alternate setting for this interface.
    .bAlternateSetting = 0,
    // The number of endpoints in this interface.
    .bNumEndpoints = 1,
    // The next three values theoretically have meaning but really they are just
    // the hoops needed to be jumped through to implement a virtual serial
    // device.
    // The interface class for this interface is CDC, indicated by 2.
    .bInterfaceClass = USB_CLASS_CDC,
    // The subclass indicates that this device uses the Abstract Control Model.
    .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
    // The protocol used by this device is AT or "Hayes Compatible".
    .bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
    // A string representing this interface. Zero means not provided.
    .iInterface = 0,

    // A pointer to the array of endpoints in this interface.
    .endpoint = cdc_comm_endpoints,
    // The extra data contains the descriptors specific to this interface's
    // function.
    .extra = &cdcacm_functional_descriptors,
    .extralen = sizeof(cdcacm_functional_descriptors)
};

static const struct usb_interface_descriptor cdc_data_interface = {
    // The size of an interface descriptor: 9
    .bLength = USB_DT_INTERFACE_SIZE,
    // A value of 4 specifies that this describes and interface.
    .bDescriptorType = USB_DT_INTERFACE,
    // The number for this interface. Starts counting from 0.
    .bInterfaceNumber = INTERFACE_CDC_DATA,
    // The number for this alternate setting for this interface.
    .bAlternateSetting = 0,
    // The number of endpoints in this interface.
    .bNumEndpoints = 2,
    // The interface class for this interface is DATA, indicated by 10.
    .bInterfaceClass = USB_CLASS_DATA,
    // There are no subclasses defined for the data class so it must be zero.
    .bInterfaceSubClass = 0,
    // We are not using any class specific protocols for data so this is set to
    // zero.
    .bInterfaceProtocol = 0,
    // A string representing this interface. Zero means not provided.
    .iInterface = 0,

    // A pointer to the array of endpoints in this interface.
    .endpoint = cdc_data_endpoints,
};

// An interface association allows the device to group a set of interfaces to
// represent one logical device to be managed by one host driver.
static const struct usb_iface_assoc_descriptor cdc_acm_interface_association = {
    // The size of an interface association descriptor: 8
    .bLength = USB_DT_INTERFACE_ASSOCIATION_SIZE,
    // A value of 11 indicates that this descriptor describes an interface
    // association.
    .bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
    // The first interface that is part of this group.
    .bFirstInterface = INTERFACE_CDC_COMM,
    // The number of included interfaces. This implies that the bundled
    // interfaces must be continugous.
    .bInterfaceCount = 2,
    // The class, subclass, and protocol of device represented by this
    // association. In this case a communication device.
    .bFunctionClass = USB_CLASS_CDC,
    // Using Abstract Control Model
    .bFunctionSubClass = USB_CDC_SUBCLASS_ACM,
    // With AT protocol (or Hayes compatible).
    .bFunctionProtocol = USB_CDC_PROTOCOL_AT,
    // A string representing this interface. Zero means not provided.
    .iFunction = 0,
};

// The endpoint for the keyboard interface.
static const struct usb_endpoint_descriptor keyboard_hid_interface_endpoint = {
    // The size of the endpoint descriptor in bytes: 7.
    .bLength = USB_DT_ENDPOINT_SIZE,
    // A value of 5 indicates that this describes an endpoint.
    .bDescriptorType = USB_DT_ENDPOINT,
    // Bit 7 indicates direction: 0 for OUT (to device) 1 for IN (to host).
    // Bits 6-4 must be set to 0.
    // Bits 3-0 indicate the endpoint number (zero is not allowed).
    // Here we define the IN side of endpoint 4.
    .bEndpointAddress = ENDPOINT_KEYBOARD_HID_IN,
    // Bit 7-2 are only used in Isochronous mode, otherwise they should be
    // 0.
    // Bit 1-0: Indicates the mode of this endpoint.
    // 00: Control
    // 01: Isochronous
    // 10: Bulk
    // 11: Interrupt
    // Here we're using interrupt.
    .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    // Maximum packet size.
    .wMaxPacketSize = 8,
    // The frequency, in number of frames, that we're going to be sending
    // data. Here we're saying we're going to send data every millisecond.
    .bInterval = 10,
};

// Keyboard Protocol 1, HID 1.11 spec, Appendix B, page 59-60
static uint8_t keyboard_hid_report_descriptor[] = {
    0x05, 0x01, // Usage Page (Generic Desktop),
    0x09, 0x06, // Usage (Keyboard),
    0xA1, 0x01, // Collection (Application),
    0x75, 0x01, //   Report Size (1),
    0x95, 0x08, //   Report Count (8),
    0x05, 0x07, //   Usage Page (Key Codes),
    0x19, 0xE0, //   Usage Minimum (224),
    0x29, 0xE7, //   Usage Maximum (231),
    0x15, 0x00, //   Logical Minimum (0),
    0x25, 0x01, //   Logical Maximum (1),
    0x81, 0x02, //   Input (Data, Variable, Absolute),  ;Modifier byte
    0x95, 0x01, //   Report Count (1),
    0x75, 0x08, //   Report Size (8),
    0x81, 0x03, //   Input (Constant),                  ;Reserved byte
    0x95, 0x05, //   Report Count (5),
    0x75, 0x01, //   Report Size (1),
    0x05, 0x08, //   Usage Page (LEDs),
    0x19, 0x01, //   Usage Minimum (1),
    0x29, 0x05, //   Usage Maximum (5),
    0x91, 0x02, //   Output (Data, Variable, Absolute), ;LED report
    0x95, 0x01, //   Report Count (1),
    0x75, 0x03, //   Report Size (3),
    0x91, 0x03, //   Output (Constant),                 ;LED report padding
    0x95, 0x06, //   Report Count (6),
    0x75, 0x08, //   Report Size (8),
    0x15, 0x00, //   Logical Minimum (0),
    0x25, 0x68, //   Logical Maximum(104),
    0x05, 0x07, //   Usage Page (Key Codes),
    0x19, 0x00, //   Usage Minimum (0),
    0x29, 0x68, //   Usage Maximum (104),
    0x81, 0x00, //   Input (Data, Array),
    0xc0        // End Collection
};

static const struct {
    struct usb_hid_descriptor hid_descriptor;
    struct {
        uint8_t bReportDescriptorType;
        uint16_t wDescriptorLength;
    } __attribute__((packed)) hid_report;
} __attribute__((packed)) keyboard_hid_function = {
    .hid_descriptor = {
        // The size of this header in bytes: 9.
        .bLength = sizeof(raw_hid_function),
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
        .wDescriptorLength = sizeof(keyboard_hid_report_descriptor),
    },
};

const struct usb_interface_descriptor keyboard_hid_interface = {
    // The size of an interface descriptor: 9
    .bLength = USB_DT_INTERFACE_SIZE,
    // A value of 4 specifies that this describes and interface.
    .bDescriptorType = USB_DT_INTERFACE,
    // The number for this interface. Starts counting from 0.
    .bInterfaceNumber = INTERFACE_KEYBOARD_HID,
    // The number for this alternate setting for this interface.
    .bAlternateSetting = 0,
    // The number of endpoints in this interface.
    .bNumEndpoints = 1,
    // The interface class for this interface is HID, defined by 3.
    .bInterfaceClass = USB_CLASS_HID,
    // The interface subclass for an HID device is used to indicate of this is a
    // mouse or keyboard that is boot mode capable (1) or not (0).
    .bInterfaceSubClass = 1, // A boot compatible device.
    .bInterfaceProtocol = 1, // A keyboard.
    // A string representing this interface. Zero means not provided.
    .iInterface = 0,
    // The header ends here.

    // A pointer to the beginning of the array of endpoints.
    .endpoint = &keyboard_hid_interface_endpoint,

    // Some class types require extra data in the interface descriptor.
    // The libopencm3 usb library requires that we stuff that here.
    // Pointer to the buffer holding the extra data.
    .extra = &keyboard_hid_function,
    // The length of the data at the above address.
    .extralen = sizeof(keyboard_hid_function),
};

const struct usb_interface interfaces[] = {
    {
        .num_altsetting = 1,
        .altsetting = &raw_hid_interface,
    },
    {
        .num_altsetting = 1,
        .iface_assoc = &cdc_acm_interface_association,
        .altsetting = &cdc_comm_interface,
    },
    {
        .num_altsetting = 1,
        .altsetting = &cdc_data_interface,
    },
    {
        .num_altsetting = 1,
        .altsetting = &keyboard_hid_interface,
    },
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
    .bNumInterfaces = INTERFACE_COUNT,
    // The index of this configuration. Starts counting from 1.
    .bConfigurationValue = 1,
    // A string index describing this configration. Zero means not provided.
    .iConfiguration = 0,
    // Bit flags:
    // 7: Must be set to 1.
    // 6: This device is self powered.
    // 5: This device supports remote wakeup.
    // 4-0: Must be set to 0.
    // TODO: Add remote wakeup.
    .bmAttributes = 0b10000000,
    // The maximum amount of current that this device will draw in 2mA units.
    // This indicates 100mA.
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

// This adds support for the additional control requests needed for the raw HID
// interface.
static int raw_hid_control_request_handler(
    usbd_device *dev,
    struct usb_setup_data *req,
    uint8_t **buf,
    uint16_t *len,
    void (**complete)(usbd_device *, struct usb_setup_data *)) {
    (void)dev;
    (void)complete;

    // This request is asking for information sent to the host using request
    // GET_DESCRIPTOR.
    if ((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN &&
        (req->bRequest == USB_REQ_GET_DESCRIPTOR)) {

        // - High byte: Descriptor type is HID report (0x22)
        // - Low byte: Index 0
        if (req->wValue == 0x2200) {
            // Send the HID report descriptor.
            *buf = (uint8_t *)raw_hid_report_descriptor;
            *len = sizeof(raw_hid_report_descriptor);
            return USBD_REQ_HANDLED;
        } else if (req->wValue == 0x2100) {
            *buf = (uint8_t *)&raw_hid_function;
            *len = sizeof(raw_hid_function);
            return USBD_REQ_HANDLED;
        }

        return USBD_REQ_NOTSUPP;
    }

    return USBD_REQ_NOTSUPP;
}

// This adds support for the additional control requests needed for the CDC
// interfaces.
static int cdcacm_control_request_handler(
    usbd_device *dev,
    struct usb_setup_data *req,
    uint8_t **buf,
    uint16_t *len,
    void (**complete)(usbd_device *, struct usb_setup_data *)) {

    (void)dev;
    (void)buf;
    (void)complete;

    if (req->bRequest == USB_CDC_REQ_SET_CONTROL_LINE_STATE) {
        // The Linux cdc_acm driver requires this to be implemented even though
        // it's optional in the CDC spec, and we don't advertise it in the ACM
        // functional descriptor.
        return USBD_REQ_HANDLED;
    } else if (req->bRequest == USB_CDC_REQ_SET_LINE_CODING) {    
        if(*len < sizeof(struct usb_cdc_line_coding)) {
            return USBD_REQ_NOTSUPP;
        }
        return USBD_REQ_HANDLED;
    }

    return USBD_REQ_NOTSUPP;
}

static struct {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} key_report;

static uint8_t keyboard_idle = 0;

// 0 is boot protocol and 1 is report protocol.
static uint8_t keyboard_protocol = 1;

static uint8_t keyboard_leds = 0;

static int keyboard_hid_control_request_handler(
    usbd_device *dev,
    struct usb_setup_data *req,
    uint8_t **buf,
    uint16_t *len,
    void (**complete)(usbd_device *, struct usb_setup_data *)) {

    (void)dev;
    (void)complete;

    // This request is asking for information sent to the host using request
    // GET_DESCRIPTOR.
    if ((req->bmRequestType & USB_REQ_TYPE_DIRECTION) == USB_REQ_TYPE_IN) {
        if ((req->bmRequestType & USB_REQ_TYPE_TYPE) == USB_REQ_TYPE_STANDARD) {
            if (req->bRequest == USB_REQ_GET_DESCRIPTOR) {
                // - High byte: Descriptor type is HID report (0x22)
                // - Low byte: Index 0
                if (req->wValue == 0x2200) {
                    // Send the HID report descriptor.
                    *buf = (uint8_t *)keyboard_hid_report_descriptor;
                    *len = sizeof(keyboard_hid_report_descriptor);
                    return USBD_REQ_HANDLED;
                } else if (req->wValue == 0x2100) {
                    *buf = (uint8_t *)&keyboard_hid_function;
                    *len = sizeof(keyboard_hid_function);
                    return USBD_REQ_HANDLED;
                }

                return USBD_REQ_NOTSUPP;
            }
        } else if ((req->bmRequestType & USB_REQ_TYPE_TYPE) == 
                   USB_REQ_TYPE_CLASS) {
            if (req->bRequest == HID_GET_REPORT) {
                *buf = (uint8_t*)&key_report;
                *len = sizeof(key_report);
                return USBD_REQ_HANDLED;
            } else if (req->bRequest == HID_GET_IDLE) {
                *buf = &keyboard_idle;
                *len = sizeof(keyboard_idle);
                return USBD_REQ_HANDLED;
            } else if (req->bRequest == HID_GET_PROTOCOL) {
                *buf = &keyboard_protocol;
                *len = sizeof(keyboard_protocol);
                return USBD_REQ_HANDLED;
            }
            
            return USBD_REQ_NOTSUPP;
        }
    } else { // IN requests
        if ((req->bmRequestType & USB_REQ_TYPE_TYPE) == USB_REQ_TYPE_CLASS) {
            if (req->bRequest == HID_SET_REPORT) {
                if (*len == 1) {
                    keyboard_leds = (*buf)[0];
                }
                return USBD_REQ_HANDLED;
            } else if (req->bRequest == HID_SET_IDLE) {
                keyboard_idle = req->wValue >> 8;
                return USBD_REQ_HANDLED;
            } else if (req->bRequest == HID_SET_PROTOCOL) {
                keyboard_protocol = req->wValue;
                return USBD_REQ_HANDLED;
            }
        }

        return USBD_REQ_NOTSUPP;
    }

    return USBD_REQ_NOTSUPP;
}

// This function is called when the target is an interface.
static int interface_control_request_handler(
    usbd_device *dev,
    struct usb_setup_data *req,
    uint8_t **buf,
    uint16_t *len,
    void (**complete)(usbd_device *, struct usb_setup_data *)) {

    if (req->wIndex == INTERFACE_RAW_HID) {
        return raw_hid_control_request_handler(dev, req, buf, len, complete);
    }

    if (req->wIndex == INTERFACE_CDC_COMM) {
        return cdcacm_control_request_handler(dev, req, buf, len, complete);
    }

    if (req->wIndex == INTERFACE_KEYBOARD_HID) {
        return keyboard_hid_control_request_handler(
            dev, req, buf, len, complete);
    }

    // This handler didn't handle this command, try the next one.
    return USBD_REQ_NEXT_CALLBACK;
}

static bool (*packet_handler)(uint8_t*);
static uint8_t hid_buffer[64];

static void hid_rx_callback(usbd_device *dev, uint8_t ep) {
    uint16_t bytes_read = usbd_ep_read_packet(
                              dev, ep, hid_buffer, sizeof(hid_buffer));
    (void)bytes_read;
    // This function reads the packet and replaces it with the response buffer.
    bool reboot = packet_handler(hid_buffer);
    // If we don't send the whole buffer then hidapi doesn't read the report.
    usbd_ep_write_packet(dev, 0x81, hid_buffer, sizeof(hid_buffer));
    if (reboot) {
        // Wait for the ack to be sent.
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
    // HID endpoints:
    // Set up endpoint 1 for data going IN to the host.
    usbd_ep_setup(
        dev, ENDPOINT_RAW_HID_IN, USB_ENDPOINT_ATTR_INTERRUPT, 64, NULL);
    // Set up endpoint 1 for data coming OUT from the host.
    usbd_ep_setup(dev, 
                  ENDPOINT_RAW_HID_OUT, 
                  USB_ENDPOINT_ATTR_INTERRUPT, 
                  64, 
                  hid_rx_callback);
    // CDC endpoints:
    // OUT endpoint for data.
    usbd_ep_setup(dev, ENDPOINT_CDC_DATA_OUT, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    // IN endpoint for data.
    usbd_ep_setup(dev, ENDPOINT_CDC_DATA_IN, USB_ENDPOINT_ATTR_BULK, 64, NULL);
    // Useless IN endpoint for comm.
    // TODO: Can this be smaller?
    usbd_ep_setup(
        dev, ENDPOINT_CDC_COMM_IN, USB_ENDPOINT_ATTR_INTERRUPT, 8, NULL);

    usbd_ep_setup(dev, 
                  ENDPOINT_KEYBOARD_HID_IN, 
                  USB_ENDPOINT_ATTR_INTERRUPT, 
                  sizeof(key_report), 
                  NULL);

    // This callback is registered for requests that are sent to an interface.
    // It does this by applying the mask to bmRequestType and making sure it is
    // equal to the supplied value.
    usbd_register_control_callback(
        dev,
        USB_REQ_TYPE_INTERFACE, // Mask
        USB_REQ_TYPE_RECIPIENT, // Value
        interface_control_request_handler); // Callback
}

// The buffer used for control requests. This needs to be big enough to hold any
// descriptor, the largest of which will be the configuration descriptor.
// TODO: confirm this is big enough by printing out totallen in the config descriptor.
static uint8_t usbd_control_buffer[256];  // TODO: 236 is my current estimate.

// Structure holding all the info related to the usb device.
static usbd_device *usbd_dev;

void usb_init(bool (*handler)(uint8_t*)) {
    packet_handler = handler;
    usbd_dev = usbd_init(&stm32f103_usb_driver, &device_descriptor,
                         &config_descriptor, usb_strings, sizeof(usb_strings),
                         usbd_control_buffer, sizeof(usbd_control_buffer));
    usbd_register_set_config_callback(usbd_dev, set_config_handler);
    nvic_enable_irq(NVIC_USB_LP_CAN_RX0_IRQ);
    // Enable USB by raising up D+ via a 1.5K resistor. This is done on the
    // WaveShare board by removing the USB EN jumper and  connecting PC0 to the
    // right hand pin of the jumper port with a patch wire. By setting PC0 to
    // open drain it turns on an NFET which pulls  up D+ via a 1.5K resistor.

    // Enable the clock to General Purpose Input Output port C.
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPCEN);
    // Set the mode for the pin. The output is zero by default, which is what
    // we want.
    gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN,
                  GPIO0);
}

uint32_t serial_usb_send_data(void *buf, int len) {
    return usbd_ep_write_packet(usbd_dev, ENDPOINT_CDC_DATA_IN, buf, len);
}

void usb_keyboard_press(uint8_t key, uint8_t modifiers) {
    key_report.modifiers = modifiers;
    key_report.keys[0] = key;
    while (usbd_ep_write_packet(usbd_dev, ENDPOINT_KEYBOARD_HID_IN, &key_report, sizeof(key_report)) == 0);
    led_toggle(1);
    key_report.modifiers = 0;
    key_report.keys[0] = 0;
    while (usbd_ep_write_packet(usbd_dev, ENDPOINT_KEYBOARD_HID_IN, &key_report, sizeof(key_report)) == 0);
    led_toggle(2);
}

// This is the interrupt handler for low priority USB events. Implementing a
// function with this name makes it the function used for the interrupt.
// TODO: Handle the other USB interrupts.
void usb_lp_can_rx0_isr(void) {
    usbd_poll(usbd_dev);
}
