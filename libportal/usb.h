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

G_BEGIN_DECLS

#define XDP_TYPE_USB_SESSION (xdp_usb_session_get_type ())

XDP_PUBLIC
G_DECLARE_FINAL_TYPE (XdpUsbSession, xdp_usb_session, XDP, USB_SESSION, GObject)

XDP_PUBLIC
GListModel *xdp_usb_session_get_devices (XdpUsbSession *self);

XDP_PUBLIC
void xdp_usb_session_close (XdpUsbSession *self);

typedef enum {
  XDP_USB_ACCESS_MODE_LISTED_DEVICES,
  XDP_USB_ACCESS_MODE_ALL,
} XdpUsbAccessMode;

XDP_PUBLIC
void           xdp_portal_create_usb_session        (XdpPortal             *portal,
                                                     XdpParent             *parent,
                                                     XdpUsbAccessMode       access_mode,
                                                     XdpUsbDeviceCandidate *candidates[],
                                                     GCancellable          *cancellable,
                                                     GAsyncReadyCallback    callback,
                                                     gpointer               data);

XDP_PUBLIC
XdpUsbSession *xdp_portal_create_usb_session_finish (XdpPortal             *portal,
                                                     GAsyncResult          *result,
                                                     GError               **error);

G_END_DECLS
