/*
 * renderer-service-upnp
 *
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Mark Ryan <mark.d.ryan@intel.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/signalfd.h>

#include "error.h"
#include "log.h"
#include "prop-defs.h"
#include "task.h"
#include "upnp.h"

#define RSU_INTERFACE_GET_VERSION "GetVersion"
#define RSU_INTERFACE_GET_SERVERS "GetServers"
#define RSU_INTERFACE_RELEASE "Release"

#define RSU_INTERFACE_FOUND_SERVER "FoundServer"
#define RSU_INTERFACE_LOST_SERVER "LostServer"

#define RSU_INTERFACE_HOST_FILE "HostFile"
#define RSU_INTERFACE_REMOVE_FILE "RemoveFile"

#define RSU_INTERFACE_VERSION "Version"
#define RSU_INTERFACE_SERVERS "Servers"

#define RSU_INTERFACE_PATH "Path"
#define RSU_INTERFACE_URI "Uri"
#define RSU_INTERFACE_ID "Id"

#define RSU_INTERFACE_GET "Get"
#define RSU_INTERFACE_GET_ALL "GetAll"
#define RSU_INTERFACE_INTERFACE_NAME "interface_name"
#define RSU_INTERFACE_PROPERTY_NAME "property_name"
#define RSU_INTERFACE_PROPERTIES_VALUE "properties"
#define RSU_INTERFACE_VALUE "value"
#define RSU_INTERFACE_OFFSET "offset"
#define RSU_INTERFACE_POSITION "position"
#define RSU_INTERFACE_TRACKID "trackid"

#define RSU_INTERFACE_RAISE "Raise"
#define RSU_INTERFACE_QUIT "Quit"
#define RSU_INTERFACE_PLAY "Play"
#define RSU_INTERFACE_PLAY_PAUSE "PlayPause"
#define RSU_INTERFACE_NEXT "Next"
#define RSU_INTERFACE_PREVIOUS "Previous"
#define RSU_INTERFACE_PAUSE "Pause"
#define RSU_INTERFACE_STOP "Stop"
#define RSU_INTERFACE_OPEN_URI "OpenUri"
#define RSU_INTERFACE_SEEK "Seek"
#define RSU_INTERFACE_SET_POSITION "SetPosition"

typedef struct rsu_context_t_ rsu_context_t;
struct rsu_context_t_ {
	bool error;
	guint rsu_id;
	guint sig_id;
	guint idle_id;
	guint owner_id;
	GDBusNodeInfo *root_node_info;
	GDBusNodeInfo *server_node_info;
	GMainLoop *main_loop;
	GDBusConnection *connection;
	gboolean quitting;
	GPtrArray *tasks;
	GHashTable *watchers;
	GCancellable *cancellable;
	rsu_upnp_t *upnp;
};

static const gchar g_rsu_root_introspection[] =
	"<node>"
	"  <interface name='"RSU_INTERFACE_MANAGER"'>"
	"    <method name='"RSU_INTERFACE_GET_VERSION"'>"
	"      <arg type='s' name='"RSU_INTERFACE_VERSION"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"RSU_INTERFACE_RELEASE"'>"
	"    </method>"
	"    <method name='"RSU_INTERFACE_GET_SERVERS"'>"
	"      <arg type='as' name='"RSU_INTERFACE_SERVERS"'"
	"           direction='out'/>"
	"    </method>"
	"    <signal name='"RSU_INTERFACE_FOUND_SERVER"'>"
	"      <arg type='s' name='"RSU_INTERFACE_PATH"'/>"
	"    </signal>"
	"    <signal name='"RSU_INTERFACE_LOST_SERVER"'>"
	"      <arg type='s' name='"RSU_INTERFACE_PATH"'/>"
	"    </signal>"
	"  </interface>"
	"</node>";

static const gchar g_rsu_server_introspection[] =
	"<node>"
	"  <interface name='"RSU_INTERFACE_PROPERTIES"'>"
	"    <method name='"RSU_INTERFACE_GET"'>"
	"      <arg type='s' name='"RSU_INTERFACE_INTERFACE_NAME"'"
	"           direction='in'/>"
	"      <arg type='s' name='"RSU_INTERFACE_PROPERTY_NAME"'"
	"           direction='in'/>"
	"      <arg type='v' name='"RSU_INTERFACE_VALUE"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"RSU_INTERFACE_GET_ALL"'>"
	"      <arg type='s' name='"RSU_INTERFACE_INTERFACE_NAME"'"
	"           direction='in'/>"
	"      <arg type='a{sv}' name='"RSU_INTERFACE_PROPERTIES_VALUE"'"
	"           direction='out'/>"
	"    </method>"
	"  </interface>"
	"  <interface name='"RSU_INTERFACE_SERVER"'>"
	"    <method name='"RSU_INTERFACE_RAISE"'>"
	"    </method>"
	"    <method name='"RSU_INTERFACE_QUIT"'>"
	"    </method>"
	"    <property type='b' name='"RSU_INTERFACE_PROP_CAN_QUIT"'"
	"       access='read'/>"
	"    <property type='b' name='"RSU_INTERFACE_PROP_CAN_RAISE"'"
	"       access='read'/>"
	"    <property type='b' name='"RSU_INTERFACE_PROP_CAN_SET_FULLSCREEN"'"
	"       access='read'/>"
	"    <property type='b' name='"RSU_INTERFACE_PROP_HAS_TRACK_LIST"'"
	"       access='read'/>"
	"    <property type='s' name='"RSU_INTERFACE_PROP_IDENTITY"'"
	"       access='read'/>"
	"    <property type='as' name='"RSU_INTERFACE_PROP_SUPPORTED_URIS"'"
	"       access='read'/>"
	"    <property type='as' name='"RSU_INTERFACE_PROP_SUPPORTED_MIME"'"
	"       access='read'/>"
	"    <property type='s' name='"RSU_INTERFACE_PROP_PROTOCOL_INFO"'"
	"       access='read'/>"
	"  </interface>"
	"  <interface name='"RSU_INTERFACE_PLAYER"'>"
	"    <method name='"RSU_INTERFACE_PLAY"'>"
	"    </method>"
	"    <method name='"RSU_INTERFACE_PAUSE"'>"
	"    </method>"
	"    <method name='"RSU_INTERFACE_PLAY_PAUSE"'>"
	"    </method>"
	"    <method name='"RSU_INTERFACE_STOP"'>"
	"    </method>"
	"    <method name='"RSU_INTERFACE_NEXT"'>"
	"    </method>"
	"    <method name='"RSU_INTERFACE_PREVIOUS"'>"
	"    </method>"
	"    <method name='"RSU_INTERFACE_OPEN_URI"'>"
	"      <arg type='s' name='"RSU_INTERFACE_URI"'"
	"           direction='in'/>"
	"    </method>"
	"    <method name='"RSU_INTERFACE_SEEK"'>"
	"      <arg type='x' name='"RSU_INTERFACE_OFFSET"'"
	"           direction='in'/>"
	"    </method>"
	"    <method name='"RSU_INTERFACE_SET_POSITION"'>"
	"      <arg type='o' name='"RSU_INTERFACE_TRACKID"'"
	"           direction='in'/>"
	"      <arg type='x' name='"RSU_INTERFACE_POSITION"'"
	"           direction='in'/>"
	"    </method>"
	"    <property type='s' name='"RSU_INTERFACE_PROP_PLAYBACK_STATUS"'"
	"       access='read'/>"
	"    <property type='d' name='"RSU_INTERFACE_PROP_RATE"'"
	"       access='read'/>"
	"    <property type='d' name='"RSU_INTERFACE_PROP_MINIMUM_RATE"'"
	"       access='read'/>"
	"    <property type='d' name='"RSU_INTERFACE_PROP_MAXIMUM_RATE"'"
	"       access='read'/>"
	"    <property type='d' name='"RSU_INTERFACE_PROP_VOLUME"'"
	"       access='read'/>"
	"    <property type='b' name='"RSU_INTERFACE_PROP_CAN_PLAY"'"
	"       access='read'/>"
	"    <property type='b' name='"RSU_INTERFACE_PROP_CAN_SEEK"'"
	"       access='read'/>"
	"    <property type='b' name='"RSU_INTERFACE_PROP_CAN_CONTROL"'"
	"       access='read'/>"
	"    <property type='b' name='"RSU_INTERFACE_PROP_CAN_PAUSE"'"
	"       access='read'/>"
	"    <property type='b' name='"RSU_INTERFACE_PROP_CAN_NEXT"'"
	"       access='read'/>"
	"    <property type='b' name='"RSU_INTERFACE_PROP_CAN_PREVIOUS"'"
	"       access='read'/>"
	"    <property type='x' name='"RSU_INTERFACE_PROP_POSITION"'"
	"       access='read'/>"
	"    <property type='a{sv}' name='"RSU_INTERFACE_PROP_METADATA"'"
	"       access='read'/>"
	"  </interface>"
	"  <interface name='"RSU_INTERFACE_PUSH_HOST"'>"
	"    <method name='"RSU_INTERFACE_HOST_FILE"'>"
	"      <arg type='s' name='"RSU_INTERFACE_PATH"'"
	"           direction='in'/>"
	"      <arg type='s' name='"RSU_INTERFACE_URI"'"
	"           direction='out'/>"
	"    </method>"
	"    <method name='"RSU_INTERFACE_REMOVE_FILE"'>"
	"      <arg type='s' name='"RSU_INTERFACE_PATH"'"
	"           direction='in'/>"
	"    </method>"
	"  </interface>"
	"</node>";


static gboolean prv_process_task(gpointer user_data);

static void prv_rsu_method_call(GDBusConnection *conn,
				const gchar *sender,
				const gchar *object,
				const gchar *interface,
				const gchar *method,
				GVariant *parameters,
				GDBusMethodInvocation *invocation,
				gpointer user_data);

static void prv_rsu_device_method_call(GDBusConnection *conn,
				       const gchar *sender,
				       const gchar *object,
				       const gchar *interface,
				       const gchar *method,
				       GVariant *parameters,
				       GDBusMethodInvocation *invocation,
				       gpointer user_data);

static void prv_props_method_call(GDBusConnection *conn,
				  const gchar *sender,
				  const gchar *object,
				  const gchar *interface,
				  const gchar *method,
				  GVariant *parameters,
				  GDBusMethodInvocation *invocation,
				  gpointer user_data);

static void prv_rsu_player_method_call(GDBusConnection *conn,
				       const gchar *sender,
				       const gchar *object,
				       const gchar *interface,
				       const gchar *method,
				       GVariant *parameters,
				       GDBusMethodInvocation *invocation,
				       gpointer user_data);

static void prv_rsu_push_host_method_call(GDBusConnection *conn,
					  const gchar *sender,
					  const gchar *object,
					  const gchar *interface,
					  const gchar *method,
					  GVariant *parameters,
					  GDBusMethodInvocation *invocation,
					  gpointer user_data);

static const GDBusInterfaceVTable g_rsu_vtable = {
	prv_rsu_method_call,
	NULL,
	NULL
};

static const GDBusInterfaceVTable g_props_vtable = {
	prv_props_method_call,
	NULL,
	NULL
};

static const GDBusInterfaceVTable g_rsu_device_vtable = {
	prv_rsu_device_method_call,
	NULL,
	NULL
};

static const GDBusInterfaceVTable g_rsu_player_vtable = {
	prv_rsu_player_method_call,
	NULL,
	NULL
};

static const GDBusInterfaceVTable g_rsu_push_host_vtable = {
	prv_rsu_push_host_method_call,
	NULL,
	NULL
};

static const GDBusInterfaceVTable *g_server_vtables[RSU_INTERFACE_INFO_MAX] = {
	&g_props_vtable,
	&g_rsu_device_vtable,
	&g_rsu_player_vtable,
	&g_rsu_push_host_vtable
};

static void prv_free_rsu_task_cb(gpointer data, gpointer user_data)
{
	rsu_task_delete(data);
}

static void prv_process_sync_task(rsu_context_t *context, rsu_task_t *task)
{
	GError *error;

	switch (task->type) {
	case RSU_TASK_GET_VERSION:
		rsu_task_complete_and_delete(task);
		break;
	case RSU_TASK_GET_SERVERS:
		task->result = rsu_upnp_get_server_ids(context->upnp);
		rsu_task_complete_and_delete(task);
		break;
	case RSU_TASK_RAISE:
	case RSU_TASK_QUIT:
		error = g_error_new(RSU_ERROR, RSU_ERROR_NOT_SUPPORTED,
				    "Command not supported.");
		rsu_task_fail_and_delete(task, error);
		g_error_free(error);
		break;
	default:
		break;
	}
}

static void prv_async_task_complete(rsu_task_t *task, GVariant *result,
				    GError *error, void *user_data)
{
	rsu_context_t *context = user_data;

	g_object_unref(context->cancellable);
	context->cancellable = NULL;

	if (error) {
		rsu_task_fail_and_delete(task, error);
		g_error_free(error);
	} else {
		task->result = result;
		rsu_task_complete_and_delete(task);
	}

	if (context->quitting)
		g_main_loop_quit(context->main_loop);
	else if (context->tasks->len > 0)
		context->idle_id = g_idle_add(prv_process_task, context);
}

static void prv_process_async_task(rsu_context_t *context, rsu_task_t *task)
{
	context->cancellable = g_cancellable_new();

	switch (task->type) {
	case RSU_TASK_GET_PROP:
		rsu_upnp_get_prop(context->upnp, task,
				  context->cancellable,
				  prv_async_task_complete, context);
		break;
	case RSU_TASK_GET_ALL_PROPS:
		rsu_upnp_get_all_props(context->upnp, task,
				       context->cancellable,
				       prv_async_task_complete, context);
		break;
	case RSU_TASK_PLAY:
		rsu_upnp_play(context->upnp, task,
			      context->cancellable,
			      prv_async_task_complete, context);
		break;
	case RSU_TASK_PAUSE:
		rsu_upnp_pause(context->upnp, task,
			      context->cancellable,
			      prv_async_task_complete, context);
		break;
	case RSU_TASK_PLAY_PAUSE:
		rsu_upnp_play_pause(context->upnp, task,
				    context->cancellable,
				    prv_async_task_complete, context);
		break;
	case RSU_TASK_STOP:
		rsu_upnp_stop(context->upnp, task,
			      context->cancellable,
			      prv_async_task_complete, context);
		break;
	case RSU_TASK_NEXT:
		rsu_upnp_next(context->upnp, task,
			      context->cancellable,
			      prv_async_task_complete, context);
		break;
	case RSU_TASK_PREVIOUS:
		rsu_upnp_previous(context->upnp, task,
				  context->cancellable,
				  prv_async_task_complete, context);
		break;
	case RSU_TASK_OPEN_URI:
		rsu_upnp_open_uri(context->upnp, task,
				  context->cancellable,
				  prv_async_task_complete, context);
		break;
	case RSU_TASK_SEEK:
		rsu_upnp_seek(context->upnp, task,
			      context->cancellable,
			      prv_async_task_complete, context);
		break;
	case RSU_TASK_SET_POSITION:
		rsu_upnp_set_position(context->upnp, task,
				      context->cancellable,
				      prv_async_task_complete, context);
		break;
	case RSU_TASK_HOST_URI:
		rsu_upnp_host_uri(context->upnp, task,
				  context->cancellable,
				  prv_async_task_complete, context);
		break;
	case RSU_TASK_REMOVE_URI:
		rsu_upnp_remove_uri(context->upnp, task,
				    context->cancellable,
				    prv_async_task_complete, context);
		break;
	default:
		break;
	}
}

static gboolean prv_process_task(gpointer user_data)
{
	rsu_context_t *context = user_data;
	rsu_task_t *task;
	gboolean retval = FALSE;

	if (context->tasks->len > 0) {
		task = g_ptr_array_index(context->tasks, 0);
		if (task->synchronous) {
			prv_process_sync_task(context, task);
			retval = TRUE;
		} else {
			prv_process_async_task(context, task);
			context->idle_id = 0;
		}
		g_ptr_array_remove_index(context->tasks, 0);
	} else {
		context->idle_id = 0;
	}

	return retval;
}

static void prv_rsu_context_init(rsu_context_t *context)
{
	memset(context, 0, sizeof(*context));
}

static void prv_rsu_context_free(rsu_context_t *context)
{
	if (context->upnp)
		rsu_upnp_delete(context->upnp);

	if (context->watchers)
		g_hash_table_unref(context->watchers);

	if (context->tasks) {
		g_ptr_array_foreach(context->tasks, prv_free_rsu_task_cb, NULL);
		g_ptr_array_unref(context->tasks);
	}

	if (context->idle_id)
		(void) g_source_remove(context->idle_id);

	if (context->sig_id)
		(void) g_source_remove(context->sig_id);

	if (context->connection) {
		if (context->rsu_id)
			g_dbus_connection_unregister_object(context->connection,
							    context->rsu_id);
	}

	if (context->main_loop)
		g_main_loop_unref(context->main_loop);

	if (context->owner_id)
		g_bus_unown_name(context->owner_id);

	if (context->connection)
		g_object_unref(context->connection);

	if (context->server_node_info)
		g_dbus_node_info_unref(context->server_node_info);

	if (context->root_node_info)
		g_dbus_node_info_unref(context->root_node_info);
}

static void prv_quit(rsu_context_t *context)
{
	if (context->cancellable) {
		g_cancellable_cancel(context->cancellable);
		context->quitting = TRUE;
	} else {
		g_main_loop_quit(context->main_loop);
	}
}

static void prv_remove_client(rsu_context_t *context, const gchar *name)
{
	rsu_upnp_lost_client(context->upnp, name);
	(void) g_hash_table_remove(context->watchers, name);

	if (g_hash_table_size(context->watchers) == 0)
		prv_quit(context);
}

static void prv_lost_client(GDBusConnection *connection, const gchar *name,
			    gpointer user_data)
{
	prv_remove_client(user_data, name);
}

static void prv_add_task(rsu_context_t *context, rsu_task_t *task)
{
	const gchar *client_name;
	guint watcher_id;

	client_name = g_dbus_method_invocation_get_sender(task->invocation);

	if (!g_hash_table_lookup(context->watchers, client_name)) {
		watcher_id = g_bus_watch_name(G_BUS_TYPE_SESSION, client_name,
					      G_BUS_NAME_WATCHER_FLAGS_NONE,
					      NULL, prv_lost_client, context,
					      NULL);

		g_hash_table_insert(context->watchers, g_strdup(client_name),
				    GUINT_TO_POINTER(watcher_id));
	}

	if (!context->cancellable && !context->idle_id)
		context->idle_id = g_idle_add(prv_process_task, context);

	g_ptr_array_add(context->tasks, task);
}

static void prv_rsu_method_call(GDBusConnection *conn,
				const gchar *sender, const gchar *object,
				const gchar *interface,
				const gchar *method, GVariant *parameters,
				GDBusMethodInvocation *invocation,
				gpointer user_data)
{
	rsu_context_t *context = user_data;
	const gchar *client_name;
	rsu_task_t *task;

	if (!strcmp(method, RSU_INTERFACE_RELEASE)) {
		client_name = g_dbus_method_invocation_get_sender(invocation);
		prv_remove_client(context, client_name);
		g_dbus_method_invocation_return_value(invocation, NULL);
	} else {
		if (!strcmp(method, RSU_INTERFACE_GET_VERSION))
			task = rsu_task_get_version_new(invocation);
		else if (!strcmp(method, RSU_INTERFACE_GET_SERVERS))
			task = rsu_task_get_servers_new(invocation);
		else
			goto finished;

		prv_add_task(context, task);
	}

finished:

	return;
}

static void prv_props_method_call(GDBusConnection *conn,
				  const gchar *sender,
				  const gchar *object,
				  const gchar *interface,
				  const gchar *method,
				  GVariant *parameters,
				  GDBusMethodInvocation *invocation,
				  gpointer user_data)
{
	rsu_context_t *context = user_data;
	rsu_task_t *task;

	if (!strcmp(method, RSU_INTERFACE_GET_ALL))
		task = rsu_task_get_props_new(invocation, object, parameters);
	else if (!strcmp(method, RSU_INTERFACE_GET))
		task = rsu_task_get_prop_new(invocation, object, parameters);
	else
		goto finished;

	prv_add_task(context, task);

finished:

	return;
}

static void prv_rsu_device_method_call(GDBusConnection *conn,
				       const gchar *sender,
				       const gchar *object,
				       const gchar *interface,
				       const gchar *method,
				       GVariant *parameters,
				       GDBusMethodInvocation *invocation,
				       gpointer user_data)

{
	rsu_context_t *context = user_data;
	rsu_task_t *task;

	if (!strcmp(method, RSU_INTERFACE_RAISE))
		task = rsu_task_raise_new(invocation);
	else if (!strcmp(method, RSU_INTERFACE_QUIT))
		task = rsu_task_quit_new(invocation);
	else
		goto finished;

	prv_add_task(context, task);

finished:

	return;
}

static void prv_rsu_player_method_call(GDBusConnection *conn,
				       const gchar *sender,
				       const gchar *object,
				       const gchar *interface,
				       const gchar *method,
				       GVariant *parameters,
				       GDBusMethodInvocation *invocation,
				       gpointer user_data)

{
	rsu_context_t *context = user_data;
	rsu_task_t *task;

	if (!strcmp(method, RSU_INTERFACE_PLAY))
		task = rsu_task_play_new(invocation, object);
	else if (!strcmp(method, RSU_INTERFACE_PAUSE))
		task = rsu_task_pause_new(invocation, object);
	else if (!strcmp(method, RSU_INTERFACE_PLAY_PAUSE))
		task = rsu_task_play_pause_new(invocation, object);
	else if (!strcmp(method, RSU_INTERFACE_STOP))
		task = rsu_task_stop_new(invocation, object);
	else if (!strcmp(method, RSU_INTERFACE_NEXT))
		task = rsu_task_next_new(invocation, object);
	else if (!strcmp(method, RSU_INTERFACE_PREVIOUS))
		task = rsu_task_previous_new(invocation, object);
	else if (!strcmp(method, RSU_INTERFACE_OPEN_URI))
		task = rsu_task_open_uri_new(invocation, object, parameters);
	else if (!strcmp(method, RSU_INTERFACE_SEEK))
		task = rsu_task_seek_new(invocation, object, parameters);
	else if (!strcmp(method, RSU_INTERFACE_SET_POSITION))
		task = rsu_task_set_position_new(invocation, object,
						 parameters);
	else
		goto finished;

	prv_add_task(context, task);

finished:

	return;
}

static void prv_rsu_push_host_method_call(GDBusConnection *conn,
					  const gchar *sender,
					  const gchar *object,
					  const gchar *interface,
					  const gchar *method,
					  GVariant *parameters,
					  GDBusMethodInvocation *invocation,
					  gpointer user_data)
{
	rsu_context_t *context = user_data;
	rsu_task_t *task;

	if (!strcmp(method, RSU_INTERFACE_HOST_FILE))
		task = rsu_task_host_uri_new(invocation, object, parameters);
	else if (!strcmp(method, RSU_INTERFACE_REMOVE_FILE))
		task = rsu_task_remove_uri_new(invocation, object, parameters);
	else
		goto on_error;

	prv_add_task(context, task);

on_error:

	return;
}

static void prv_found_media_server(const gchar *path, void *user_data)
{
	rsu_context_t *context = user_data;

	(void) g_dbus_connection_emit_signal(context->connection,
					     NULL,
					     RSU_OBJECT,
					     RSU_INTERFACE_MANAGER,
					     RSU_INTERFACE_FOUND_SERVER,
					     g_variant_new("(s)", path),
					     NULL);
}

static void prv_lost_media_server(const gchar *path, void *user_data)
{
	rsu_context_t *context = user_data;

	(void) g_dbus_connection_emit_signal(context->connection,
					     NULL,
					     RSU_OBJECT,
					     RSU_INTERFACE_MANAGER,
					     RSU_INTERFACE_LOST_SERVER,
					     g_variant_new("(s)", path),
					     NULL);
}

static void prv_bus_acquired(GDBusConnection *connection, const gchar *name,
			     gpointer user_data)
{
	rsu_context_t *context = user_data;
	unsigned int i;
	rsu_interface_info_t *info;

	context->connection = connection;

	context->rsu_id =
		g_dbus_connection_register_object(connection, RSU_OBJECT,
						  context->root_node_info->
						  interfaces[0],
						  &g_rsu_vtable,
						  user_data, NULL, NULL);

	if (!context->rsu_id) {
		context->error = true;
		g_main_loop_quit(context->main_loop);
	} else {
		info = g_new0(rsu_interface_info_t, RSU_INTERFACE_INFO_MAX);

		for (i = 0; i < RSU_INTERFACE_INFO_MAX; ++i) {
			info[i].interface =
				context->server_node_info->interfaces[i];
			info[i].vtable = g_server_vtables[i];
		}

		context->upnp = rsu_upnp_new(connection, info,
					     prv_found_media_server,
					     prv_lost_media_server,
					     user_data);
	}
}

static void prv_name_lost(GDBusConnection *connection, const gchar *name,
			  gpointer user_data)
{
	rsu_context_t *context = user_data;

	context->connection = NULL;

	prv_quit(context);
}

static gboolean prv_quit_handler(GIOChannel *source, GIOCondition condition,
				 gpointer user_data)
{
	rsu_context_t *context = user_data;

	prv_quit(context);
	context->sig_id = 0;

	return FALSE;
}

static bool prv_init_signal_handler(sigset_t mask, rsu_context_t *context)
{
	bool retval = false;
	int fd = -1;
	GIOChannel *channel = NULL;

	fd = signalfd(-1, &mask, SFD_NONBLOCK);

	if (fd == -1)
		goto on_error;

	channel = g_io_channel_unix_new(fd);
	g_io_channel_set_close_on_unref(channel, TRUE);

	if (g_io_channel_set_flags(channel, G_IO_FLAG_NONBLOCK, NULL) !=
	    G_IO_STATUS_NORMAL)
		goto on_error;

	if (g_io_channel_set_encoding(channel, NULL, NULL) !=
	    G_IO_STATUS_NORMAL)
		goto on_error;

	context->sig_id = g_io_add_watch(channel, G_IO_IN | G_IO_PRI,
					 prv_quit_handler,
					 context);

	retval = true;

on_error:

	if (channel)
		g_io_channel_unref(channel);

	return retval;
}

static void prv_unregister_client(gpointer client)
{
	guint id = GPOINTER_TO_UINT(client);
	g_bus_unwatch_name(id);
}

int main(int argc, char *argv[])
{
	rsu_context_t context;
	rsu_log_t log_context;
	sigset_t mask;
	int retval = 1;

	prv_rsu_context_init(&context);

	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGINT);

	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
		goto on_error;

	g_type_init();

	rsu_log_init(argv[0], &log_context);

	context.root_node_info =
		g_dbus_node_info_new_for_xml(g_rsu_root_introspection, NULL);

	if (!context.root_node_info)
		goto on_error;

	context.server_node_info =
		g_dbus_node_info_new_for_xml(g_rsu_server_introspection, NULL);

	if (!context.server_node_info)
		goto on_error;

	context.main_loop = g_main_loop_new(NULL, FALSE);

	context.owner_id = g_bus_own_name(G_BUS_TYPE_SESSION,
					  RSU_SERVER_NAME,
					  G_BUS_NAME_OWNER_FLAGS_NONE,
					  prv_bus_acquired, NULL,
					  prv_name_lost, &context, NULL);

	context.tasks = g_ptr_array_new();

	context.watchers = g_hash_table_new_full(g_str_hash, g_str_equal,
						 g_free, prv_unregister_client);

	if (!prv_init_signal_handler(mask, &context))
		goto on_error;

	g_main_loop_run(context.main_loop);

	if (context.error)
		goto on_error;

	retval = 0;

on_error:

	rsu_log_finialize(&log_context);

	prv_rsu_context_free(&context);

	return retval;
}
