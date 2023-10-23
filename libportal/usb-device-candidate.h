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

#define XDP_TYPE_USB_DEVICE_CANDIDATE (xdp_usb_device_candidate_get_type ())
typedef struct _XdpUsbDeviceCandidate XdpUsbDeviceCandidate;

XDP_PUBLIC
GType xdp_usb_device_candidate_get_type (void) G_GNUC_CONST;

XDP_PUBLIC
XdpUsbDeviceCandidate *
xdp_usb_device_candidate_new (uint16_t vendor_id,
                              uint16_t product_id);

XDP_PUBLIC
XdpUsbDeviceCandidate *
xdp_usb_device_candidate_copy (XdpUsbDeviceCandidate *self);

XDP_PUBLIC
void xdp_usb_device_candidate_free (XdpUsbDeviceCandidate *self);

XDP_PUBLIC
uint16_t xdp_usb_device_candidate_get_vendor_id (XdpUsbDeviceCandidate *self);

XDP_PUBLIC
uint16_t xdp_usb_device_candidate_get_product_id (XdpUsbDeviceCandidate *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (XdpUsbDeviceCandidate, xdp_usb_device_candidate_free)

G_END_DECLS

