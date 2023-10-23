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

#include "usb.h"
#include "portal-private.h"
#include "session-private.h"
#include "usb-device-private.h"
#include "usb-device-candidate-private.h"

struct _XdpUsbSession
{
  GObject parent_instance;
  XdpSession *parent_session; /* owned */
  guint device_events_signal_id;
  GListStore *devices;
};

G_DEFINE_TYPE (XdpUsbSession, xdp_usb_session, G_TYPE_OBJECT)

enum {
  CLOSED,
  N_SIGNALS,
};

static guint signals[N_SIGNALS];

static void
close_parent_session (XdpUsbSession *self)
{
  if (!self->parent_session)
    return;

  g_debug ("Closing USB session %p", self);

  g_dbus_connection_signal_unsubscribe (self->parent_session->portal->bus,
                                        self->device_events_signal_id);
  self->device_events_signal_id = 0;

  g_list_store_remove_all (self->devices);

  xdp_session_close (self->parent_session);
  g_clear_object (&self->parent_session);
}

static void
usb_session_device_events_cb (GDBusConnection *bus,
                              const char      *sender_name,
                              const char      *object_path,
                              const char      *interface_name,
                              const char      *signal_name,
                              GVariant        *parameters,
                              gpointer         data)
{
  g_autoptr(GVariant) devices = NULL;
  XdpUsbSession *self;
  GVariantIter *device_iter;
  GVariantIter iter;
  const char *event;
  const char *id;

  self = XDP_USB_SESSION (data);
  devices = g_variant_get_child_value (parameters, 1);

  g_variant_iter_init (&iter, devices);
  while (g_variant_iter_next (&iter, "(&s&sa{sv})", &event, &id, &device_iter))
    {
      g_assert (event != NULL);
      g_assert (id != NULL);

      g_debug ("[usb] (DeviceEvents): event: %s, id: %s", event, id);

      if (g_strcmp0 (event, "add") == 0)
        {
          g_autoptr(XdpUsbDevice) device = xdp_usb_device_new (id, device_iter);
          g_list_store_append (self->devices, device);
        }
      else if (g_strcmp0 (event, "remove") == 0)
        {
          GListModel *model = G_LIST_MODEL (self->devices);

          for (unsigned int i = 0; i < g_list_model_get_n_items (model); i++)
            {
              g_autoptr(XdpUsbDevice) device = g_list_model_get_item (model, i);

              if (g_strcmp0 (id, xdp_usb_device_get_id (device)) == 0)
                {
                  g_list_store_remove (self->devices, i);
                  break;
                }
            }
        }
    }
}

static void
on_parent_session_closed_cb (XdpSession    *session,
                             XdpUsbSession *self)
{
  g_signal_emit (self, signals[CLOSED], 0);
}

static void
xdp_usb_session_dispose (GObject *object)
{
  XdpUsbSession *self = XDP_USB_SESSION (object);

  close_parent_session (self);

  g_clear_object (&self->devices);

  G_OBJECT_CLASS (xdp_usb_session_parent_class)->dispose (object);
}

static void
xdp_usb_session_class_init (XdpUsbSessionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = xdp_usb_session_dispose;

  /**
   * XdpUsbSession::closed:
   * @session: the [class@UsbSession]
   *
   * Emitted when @session was closed, either by using [method@UsbSession.close]
   * or when closed by XDG desktop portal.
   */
  signals[CLOSED] =
    g_signal_new ("closed",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_CLEANUP | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);
}

static void
xdp_usb_session_init (XdpUsbSession *session)
{
  session->devices = g_list_store_new (XDP_TYPE_USB_DEVICE);
}

static XdpUsbSession *
xdp_usb_session_new (XdpPortal    *portal,
                     const char   *session_handle,
                     GVariantIter *devices_iter)
{
  g_autoptr(XdpUsbSession) usb_session = NULL;
  GVariantIter *device_iter;
  const char *id;

  g_debug ("Creating USB session from handle %s", session_handle);

  usb_session = g_object_new (XDP_TYPE_USB_SESSION, NULL);
  usb_session->parent_session = _xdp_session_new (portal, session_handle, XDP_SESSION_USB);
  g_signal_connect (usb_session->parent_session,
                    "closed",
                    G_CALLBACK (on_parent_session_closed_cb),
                    usb_session);

  usb_session->device_events_signal_id =
    g_dbus_connection_signal_subscribe (portal->bus,
                                        PORTAL_BUS_NAME,
                                        "org.freedesktop.portal.Usb",
                                        "DeviceEvents",
                                        session_handle,
                                        NULL,
                                        G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                        usb_session_device_events_cb,
                                        usb_session,
                                        NULL);

  while (devices_iter &&
         g_variant_iter_next (devices_iter, "(&sa{sv})", &id, &device_iter))
    {
      g_autoptr(XdpUsbDevice) device = xdp_usb_device_new (id, device_iter);
      g_list_store_append (usb_session->devices, device);
    }

  return g_steal_pointer (&usb_session);
}

/**
 * xdp_usb_session_get_devices:
 * @self: a [class@UsbSession]
 *
 * Retrieves the list of devices that @self has access to.
 *
 * Returns: (transfer full): a [class@Gio.ListModel]
 *  containing [class@UsbDevice]s.
 */
GListModel *
xdp_usb_session_get_devices (XdpUsbSession *self)
{
  g_return_val_if_fail (XDP_IS_USB_SESSION (self), NULL);

  return (GListModel *) g_object_ref (self->devices);
}

/**
 * xdp_usb_session_close:
 * @self: a [class@UsbSession]
 *
 * Closes the USB session represented by @self. Does nothing if
 * @self is already closed.
 */
void
xdp_usb_session_close (XdpUsbSession *self)
{
  g_return_if_fail (XDP_IS_USB_SESSION (self));

  close_parent_session (self);
}

/* CreateSession */

typedef struct {
  XdpPortal *portal;
  char *session_id;
  guint signal_id;
  GCancellable *cancellable;
  XdpParent *parent;
  char *parent_handle;
  GTask *task;
  char *request_path;
  guint cancelled_id;
  GPtrArray *device_candidates;
  XdpUsbAccessMode access_mode;
  char *reason;
} CreateUsbSessionCall;

static const char *
access_mode_to_string (XdpUsbAccessMode access_mode)
{
  switch (access_mode)
    {
    case XDP_USB_ACCESS_MODE_LISTED_DEVICES:
      return "listed-devices";
    case XDP_USB_ACCESS_MODE_ALL:
      return "all";
    default:
      return NULL;
    }
}

static void
create_usb_session_call_free (CreateUsbSessionCall *call)
{
  if (!call)
    return;

  if (call->parent)
    {
      call->parent->parent_unexport (call->parent);
      g_clear_pointer (&call->parent, xdp_parent_free);
    }
  g_clear_pointer (&call->parent_handle, g_free);

  if (call->signal_id)
    g_dbus_connection_signal_unsubscribe (call->portal->bus, call->signal_id);

  if (call->cancelled_id)
    g_signal_handler_disconnect (call->cancellable, call->cancelled_id);

  g_clear_pointer (&call->device_candidates, g_ptr_array_unref);
  g_clear_pointer (&call->request_path, g_free);
  g_clear_pointer (&call->session_id, g_free);
  g_clear_object (&call->cancellable);
  g_clear_object (&call->portal);
  g_clear_object (&call->task);
  g_free (call);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (CreateUsbSessionCall, create_usb_session_call_free)

static void
create_usb_session_response_received (GDBusConnection *bus,
                                      const char      *sender_name,
                                      const char      *object_path,
                                      const char      *interface_name,
                                      const char      *signal_name,
                                      GVariant        *parameters,
                                      gpointer         data)
{
  g_autoptr(CreateUsbSessionCall) call = data;
  g_autoptr(GVariant) ret = NULL;
  guint32 response;

  if (call->cancelled_id)
    {
      g_signal_handler_disconnect (call->cancellable, call->cancelled_id);
      call->cancelled_id = 0;
    }

  g_variant_get (parameters, "(u@a{sv})", &response, &ret);

  if (response == 0)
    {
      g_autoptr(XdpUsbSession) usb_session = NULL;
      g_autoptr(GVariantIter) available_devices = NULL;

      g_variant_lookup (ret, "available_devices", "a(sa{sv})", &available_devices);

      usb_session = xdp_usb_session_new (call->portal, call->session_id, available_devices);
      g_task_return_pointer (call->task, g_steal_pointer (&usb_session), g_object_unref);
    }
  else if (response == 1)
    {
      g_task_return_new_error (call->task, G_IO_ERROR, G_IO_ERROR_CANCELLED, "USB permission request canceled");
    }
  else
    {
      g_task_return_new_error (call->task, G_IO_ERROR, G_IO_ERROR_FAILED, "USB permission request failed");
    }
}

static void
create_usb_session_cancelled_cb (GCancellable *cancellable,
                                 gpointer      data)
{
  g_autoptr(CreateUsbSessionCall) call = data;

  g_debug ("Calling Close");
  g_dbus_connection_call (call->portal->bus,
                          PORTAL_BUS_NAME,
                          call->request_path,
                          REQUEST_INTERFACE,
                          "Close",
                          NULL,
                          NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL, NULL, NULL);

  g_task_return_new_error (call->task, G_IO_ERROR, G_IO_ERROR_CANCELLED,
                           "CreateUsbSession call canceled by caller");
}

static void
create_usb_sesion_returned (GObject      *object,
                            GAsyncResult *result,
                            gpointer      data)
{
  CreateUsbSessionCall *call = data;
  g_autoptr(GVariant) ret = NULL;
  g_autoptr(GError) error = NULL;

  ret = g_dbus_connection_call_finish (G_DBUS_CONNECTION (object), result, &error);
  if (error)
    {
      g_task_return_error (call->task, g_steal_pointer (&error));
      g_clear_pointer (&call, create_usb_session_call_free);
    }
}

static void create_usb_session (CreateUsbSessionCall *call);

static void
parent_exported (XdpParent  *parent,
                 const char *handle,
                 gpointer    data)
{
  CreateUsbSessionCall *call = data;
  call->parent_handle = g_strdup (handle);
  create_usb_session (call);
}

static void
create_usb_session (CreateUsbSessionCall *call)
{
  g_autofree char *session_token = NULL;
  g_autofree char *token = NULL;
  GVariantBuilder options;

  if (call->parent_handle == NULL)
    {
      call->parent->parent_export (call->parent, parent_exported, call);
      return;
    }

  token = g_strdup_printf ("portal%d", g_random_int_range (0, G_MAXINT));
  call->request_path = g_strconcat (REQUEST_PATH_PREFIX, call->portal->sender, "/", token, NULL);
  call->signal_id = g_dbus_connection_signal_subscribe (call->portal->bus,
                                                        PORTAL_BUS_NAME,
                                                        REQUEST_INTERFACE,
                                                        "Response",
                                                        call->request_path,
                                                        NULL,
                                                        G_DBUS_SIGNAL_FLAGS_NO_MATCH_RULE,
                                                        create_usb_session_response_received,
                                                        call,
                                                        NULL);

  if (call->cancellable)
    {
      call->cancelled_id = g_signal_connect (call->cancellable,
                                             "cancelled",
                                             G_CALLBACK (create_usb_session_cancelled_cb),
                                             call);
    }

  session_token = g_strdup_printf ("portal%d", g_random_int_range (0, G_MAXINT));
  call->session_id = g_strconcat (SESSION_PATH_PREFIX, call->portal->sender, "/", session_token, NULL);

  g_variant_builder_init (&options, G_VARIANT_TYPE_VARDICT);
  g_variant_builder_add (&options, "{sv}", "handle_token", g_variant_new_string (token));
  g_variant_builder_add (&options, "{sv}", "session_handle_token", g_variant_new_string (session_token));
  g_variant_builder_add (&options, "{sv}", "access_mode", g_variant_new_string (access_mode_to_string (call->access_mode)));
  if (call->reason)
    g_variant_builder_add (&options, "{sv}", "reason", g_variant_new_string (call->reason));

  if (call->device_candidates)
    {
      GVariantBuilder devices;

      g_variant_builder_init (&devices, G_VARIANT_TYPE ("aa{sv}"));

      for (unsigned int i = 0; i < call->device_candidates->len; i++)
        {
          XdpUsbDeviceCandidate *candidate = g_ptr_array_index (call->device_candidates, i);
          g_variant_builder_add (&devices, "a{sv}", xdp_usb_device_candidate_to_gvariant (candidate));
        }

      g_variant_builder_add (&options, "{sv}", "devices", g_variant_builder_end (&devices));
    }

  g_debug ("Calling USB CreateSession");
  g_dbus_connection_call (call->portal->bus,
                          PORTAL_BUS_NAME,
                          PORTAL_OBJECT_PATH,
                          "org.freedesktop.portal.Usb",
                          "CreateSession",
                          g_variant_new ("(sa{sv})",
                                         call->parent_handle,
                                         &options),
                          NULL,
                          G_DBUS_CALL_FLAGS_NONE,
                          -1,
                          NULL,
                          create_usb_sesion_returned,
                          call);
}

/**
 * xdp_portal_create_usb_session:
 * @portal: a [class@Portal]
 * @parent: (nullable): parent window information
 * @access_mode: a #XdpUsbAccessMode
 * @candidates: (nullable)(array zero-terminated=1): a NULL-terminated array
 *   of [class@UsbDeviceCandidate]s
 * @cancellable: (nullable): optional [class@Gio.Cancellable]
 * @callback: (scope async): a callback to call when the request is done
 * @data: (closure): data to pass to @callback
 *
 * Creates a USB session.
 *
 * When the request is done, @callback will be called.
 * You can then call [method@Portal.create_usb_session_finish]
 * to get the results.
 */
void
xdp_portal_create_usb_session (XdpPortal             *portal,
                               XdpParent             *parent,
                               XdpUsbAccessMode       access_mode,
                               XdpUsbDeviceCandidate *candidates[],
                               GCancellable          *cancellable,
                               GAsyncReadyCallback    callback,
                               gpointer               data)
{
  CreateUsbSessionCall *call;

  g_return_if_fail (XDP_IS_PORTAL (portal));
  g_return_if_fail (!candidates || candidates[0] != NULL);

  call = g_new0 (CreateUsbSessionCall, 1);
  call->portal = g_object_ref (portal);
  if (parent)
    call->parent = xdp_parent_copy (parent);
  else
    call->parent_handle = g_strdup ("");
  if (cancellable)
    call->cancellable = g_object_ref (cancellable);
  call->access_mode = access_mode;
  call->reason = NULL; /* TODO */

  if (candidates)
    {
      call->device_candidates =
        g_ptr_array_new_with_free_func ((GDestroyNotify) xdp_usb_device_candidate_free);

      for (unsigned int i = 0; candidates[i] != NULL; i++)
        g_ptr_array_add (call->device_candidates, xdp_usb_device_candidate_copy (candidates[i]));
    }

  call->task = g_task_new (portal, NULL, callback, data);
  g_task_set_static_name (call->task, "[usb] xdp_portal_create_usb_session");
  g_task_set_source_tag (call->task, xdp_portal_create_usb_session);

  create_usb_session (call);
}

/**
 * xdp_portal_create_usb_session_finish:
 * @portal: a [class@Portal]
 * @result: a [iface@Gio.AsyncResult]
 * @error: return location for an error
 *
 * Finishes creating an USB session.
 *
 * If the access was granted, you can then call
 * [method@Portal.create_usb_session] to obtain a list of
 * USB devices remote.
 *
 * Returns: (transfer full)(nullable): an #XdpUsbSession, or
 *  %NULL if call failed
 */
XdpUsbSession *
xdp_portal_create_usb_session_finish (XdpPortal     *portal,
                                      GAsyncResult  *result,
                                      GError       **error)
{
  g_return_val_if_fail (XDP_IS_PORTAL (portal), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, portal), FALSE);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == xdp_portal_create_usb_session, FALSE);

  return g_task_propagate_pointer (G_TASK (result), error);
}
