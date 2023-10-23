/*
 * Copyright (C) 2023 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 3.0 of the
 * License.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0-only
 */

#pragma once

#include <libportal/types.h>
#include <stdint.h>

G_BEGIN_DECLS

#define XDP_TYPE_USB_DEVICE (xdp_usb_device_get_type())

XDP_PUBLIC
G_DECLARE_FINAL_TYPE (XdpUsbDevice, xdp_usb_device, XDP, USB_DEVICE, GObject)

XDP_PUBLIC
const char *xdp_usb_device_get_id (XdpUsbDevice *self);

XDP_PUBLIC
const char *xdp_usb_device_get_property_string (XdpUsbDevice *self,
                                                const char   *property);

XDP_PUBLIC
gboolean xdp_usb_device_get_property_boolean (XdpUsbDevice *self,
                                              const char   *property);

XDP_PUBLIC
uint16_t xdp_usb_device_get_property_uint16 (XdpUsbDevice *self,
                                             const char   *property);

G_END_DECLS
