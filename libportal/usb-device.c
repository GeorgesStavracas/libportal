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

#include "usb-device.h"

struct _XdpUsbDevice
{
  GObject parent_instance;

  char *id;
  GVariantDict *properties;
};

G_DEFINE_FINAL_TYPE (XdpUsbDevice, xdp_usb_device, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_ID,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];

static void
xdp_usb_device_finalize (GObject *object)
{
  XdpUsbDevice *self = (XdpUsbDevice *)object;

  g_clear_pointer (&self->properties, g_variant_dict_unref);
  g_clear_pointer (&self->id, g_free);

  G_OBJECT_CLASS (xdp_usb_device_parent_class)->finalize (object);
}

static void
xdp_usb_device_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  XdpUsbDevice *self = XDP_USB_DEVICE (object);

  switch (prop_id)
    {
    case PROP_ID:
      g_value_set_string (value, self->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xdp_usb_device_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  XdpUsbDevice *self = XDP_USB_DEVICE (object);

  switch (prop_id)
    {
    case PROP_ID:
      g_assert (self->id == NULL);
      self->id = g_value_dup_string (value);
      g_assert (self->id != NULL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
xdp_usb_device_class_init (XdpUsbDeviceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = xdp_usb_device_finalize;
  object_class->get_property = xdp_usb_device_get_property;
  object_class->set_property = xdp_usb_device_set_property;

  properties[PROP_ID] =
    g_param_spec_string ("id", "id", "USB device identifier", NULL,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
xdp_usb_device_init (XdpUsbDevice *self)
{
  self->properties = g_variant_dict_new (NULL);
}

/**
 * xdp_usb_device_get_id:
 * @self: a #XdpUsbDevice
 *
 * Retrieves the 'id' property of @self.
 *
 * Returns: (transfer none): a string
 */
const char *
xdp_usb_device_get_id (XdpUsbDevice *self)
{
  g_return_val_if_fail (XDP_IS_USB_DEVICE (self), NULL);

  return self->id;
}

/**
 * xdp_usb_device_get_property_string:
 * @self: a #XdpUsbDevice
 * @property: a property name
 *
 * Retrieves the property @property of @self.
 *
 * It is a programming error to retrieve a non-string
 * property with this getter.
 *
 * Returns: (transfer none)(nullable): a string
 */
const char *
xdp_usb_device_get_property_string (XdpUsbDevice *self,
                                    const char   *property)
{
  const char *value = NULL;

  g_return_val_if_fail (XDP_IS_USB_DEVICE (self), NULL);
  g_return_val_if_fail (property != NULL && g_utf8_validate (property, -1, NULL), NULL);

  g_variant_dict_lookup (self->properties, property, "&s", &value);

  return value;
}

/**
 * xdp_usb_device_get_property_boolean:
 * @self: a #XdpUsbDevice
 * @property: a property name
 *
 * Retrieves the boolean property @property of @self.
 *
 * It is a programming error to retrieve a non-boolean
 * property with this getter.
 *
 * Returns: a boolean
 */
gboolean
xdp_usb_device_get_property_boolean (XdpUsbDevice *self,
                                     const char   *property)
{
  gboolean value = FALSE;

  g_return_val_if_fail (XDP_IS_USB_DEVICE (self), FALSE);
  g_return_val_if_fail (property != NULL && g_utf8_validate (property, -1, NULL), FALSE);

  g_variant_dict_lookup (self->properties, property, "b", &value);

  return value;
}

/**
 * xdp_usb_device_get_property_uint16:
 * @self: a #XdpUsbDevice
 * @property: a property name
 *
 * Retrieves the boolean property @property of @self.
 *
 * It is a programming error to retrieve a non-boolean
 * property with this getter.
 *
 * Returns: an unsigned 16-bit integer
 */
uint16_t
xdp_usb_device_get_property_uint16 (XdpUsbDevice *self,
                                    const char   *property)
{
  uint16_t value = -1;

  g_return_val_if_fail (XDP_IS_USB_DEVICE (self), -1);
  g_return_val_if_fail (property != NULL && g_utf8_validate (property, -1, NULL), -1);

  g_variant_dict_lookup (self->properties, property, "q", &value);

  return value;
}

XdpUsbDevice *
xdp_usb_device_new (const char   *id,
                    GVariantIter *properties_iter)
{
  g_autoptr(XdpUsbDevice) usb_device = NULL;
  GVariant *value;
  const char *key;

  usb_device = g_object_new (XDP_TYPE_USB_DEVICE,
                             "id", id,
                             NULL);

  while (g_variant_iter_next (properties_iter, "{&sv}", &key, &value))
    {
      g_variant_dict_insert_value (usb_device->properties, key, value);
      g_message ("    Property: %s → %s", key, g_variant_print (value, TRUE));
    }

  return g_steal_pointer (&usb_device);
}

