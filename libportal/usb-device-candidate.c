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

#include "config.h"

#include "usb-device-candidate.h"

G_DEFINE_BOXED_TYPE (XdpUsbDeviceCandidate,
                     xdp_usb_device_candidate,
                     xdp_usb_device_candidate_copy,
                     xdp_usb_device_candidate_free)

struct _XdpUsbDeviceCandidate
{
  uint16_t vendor_id;
  uint16_t product_id;
};

/**
 * xdp_usb_device_candidate_new: (constructor)
 * @vendor_id: an USB vendor id
 * @product_id: an USB product id
 *
 * Create a #XdpUsbDeviceCandidate with @vendor_id and @product_id.
 *
 * Returns: (transfer full): A newly created #XdpUsbDeviceCandidate
 */
XdpUsbDeviceCandidate *
xdp_usb_device_candidate_new (uint16_t vendor_id,
                              uint16_t product_id)
{
  XdpUsbDeviceCandidate *self;

  self = g_new0 (XdpUsbDeviceCandidate, 1);
  self->vendor_id = vendor_id;
  self->product_id = product_id;

  return self;
}

/**
 * xdp_usb_device_candidate_copy:
 * @self: a #XdpUsbDeviceCandidate
 *
 * Makes a deep copy of a #XdpUsbDeviceCandidate.
 *
 * Returns: (transfer full): A newly created #XdpUsbDeviceCandidate with the same
 *   contents as @self
 */
XdpUsbDeviceCandidate *
xdp_usb_device_candidate_copy (XdpUsbDeviceCandidate *self)
{
  XdpUsbDeviceCandidate *copy;

  g_return_val_if_fail (self, NULL);

  copy = g_new0 (XdpUsbDeviceCandidate, 1);
  copy->vendor_id = self->vendor_id;
  copy->product_id = self->product_id;

  return copy;
}

/**
 * xdp_usb_device_candidate_free:
 * @self: a #XdpUsbDeviceCandidate
 *
 * Frees a #XdpUsbDeviceCandidate allocated using xdp_usb_device_candidate_new()
 * or xdp_usb_device_candidate_copy().
 */
void
xdp_usb_device_candidate_free (XdpUsbDeviceCandidate *self)
{
  g_return_if_fail (self);

  g_free (self);
}

/**
 * xdp_usb_device_candidate_get_vendor_id:
 * @self: a #XdpUsbDeviceCandidate
 *
 * Retrieves the USB vendor id of @self.
 *
 * Returns: the USB vendor id
 */
uint16_t
xdp_usb_device_candidate_get_vendor_id (XdpUsbDeviceCandidate *self)
{
  g_return_val_if_fail (self, 0);

  return self->vendor_id;
}

/**
 * xdp_usb_device_candidate_get_product_id:
 * @self: a #XdpUsbDeviceCandidate
 *
 * Retrieves the USB product id of @self.
 *
 * Returns: the USB product id
 */
uint16_t
xdp_usb_device_candidate_get_product_id (XdpUsbDeviceCandidate *self)
{
  g_return_val_if_fail (self, 0);

  return self->product_id;
}

GVariant *
xdp_usb_device_candidate_to_gvariant (XdpUsbDeviceCandidate *self)
{
  GVariantBuilder builder;

  g_return_val_if_fail (self, NULL);

  g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&builder, "{sv}", "vendor_id", g_variant_new_int16 (self->vendor_id));
  g_variant_builder_add (&builder, "{sv}", "product_id", g_variant_new_int16 (self->product_id));

  return g_variant_builder_end (&builder);
}
