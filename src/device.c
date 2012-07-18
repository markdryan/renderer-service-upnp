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


#include <string.h>

#include <libgupnp/gupnp-control-point.h>
#include <libgupnp-av/gupnp-av.h>

#include "async.h"
#include "device.h"
#include "error.h"
#include "prop-defs.h"

typedef void (*rsu_device_local_cb_t)(rsu_async_cb_data_t *cb_data);

typedef struct rsu_device_data_t_ rsu_device_data_t;
struct rsu_device_data_t_ {
	rsu_device_local_cb_t local_cb;
};

static void prv_last_change_cb(GUPnPServiceProxy *proxy,
			       const char *variable,
			       GValue *value,
			       gpointer user_data);

static void prv_sink_change_cb(GUPnPServiceProxy *proxy,
			       const char *variable,
			       GValue *value,
			       gpointer user_data);

static void prv_unref_variant(gpointer variant)
{
	GVariant *var = variant;
	if (var)
		g_variant_unref(var);
}

static void prv_props_init(rsu_props_t *props)
{
	props->root_props = g_hash_table_new_full(g_str_hash, g_str_equal,
						  NULL, prv_unref_variant);
	props->player_props = g_hash_table_new_full(g_str_hash, g_str_equal,
						    NULL, prv_unref_variant);
	props->synced = FALSE;
}

static void prv_props_free(rsu_props_t *props)
{
	g_hash_table_unref(props->root_props);
	g_hash_table_unref(props->player_props);
}

static void prv_service_proxies_free(rsu_service_proxies_t *service_proxies)
{
	if (service_proxies->av_proxy)
		g_object_unref(service_proxies->av_proxy);
	if (service_proxies->rc_proxy)
		g_object_unref(service_proxies->rc_proxy);
	if (service_proxies->cm_proxy)
		g_object_unref(service_proxies->cm_proxy);
}

static void prv_rsu_context_delete(gpointer context)
{
	rsu_device_context_t *ctx = context;
	rsu_service_proxies_t *service_proxies;

	if (ctx) {
		service_proxies = &ctx->service_proxies;

		(void) gupnp_service_proxy_remove_notify(
			service_proxies->cm_proxy ,"SinkProtocolInfo",
			prv_sink_change_cb, ctx->device);

		(void) gupnp_service_proxy_remove_notify(
			service_proxies->av_proxy ,"LastChange",
			prv_last_change_cb, ctx->device);

		g_free(ctx->ip_address);
		if (ctx->device_proxy)
			g_object_unref(ctx->device_proxy);
		prv_service_proxies_free(&ctx->service_proxies);
		g_free(ctx);
	}
}

static void prv_merge_meta_data(rsu_device_t *device, const gchar *key,
				GVariant *value)
{
	GVariant *current_meta_data;
	GVariantIter viter;
	GVariantBuilder *vb;
	GVariant *val;
	gchar *vkey;
	gboolean replaced = FALSE;
	GVariant *new_val;

	vb = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));

	current_meta_data = g_hash_table_lookup(device->props.player_props,
						RSU_INTERFACE_PROP_METADATA);
	if (current_meta_data) {
		g_variant_iter_init(&viter, current_meta_data);
		while (g_variant_iter_next(&viter, "{&sv}", &vkey, &val)) {
			if (!strcmp(key, vkey)) {
				new_val = value;
				replaced = TRUE;
			} else {
				new_val = val;
			}
			g_variant_builder_add(vb, "{sv}", vkey, new_val);
			g_variant_unref(val);
		}
	}

	if (!replaced)
		g_variant_builder_add(vb, "{sv}", key, value);

	val = g_variant_ref_sink(g_variant_builder_end(vb));
	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_METADATA, val);
	g_variant_builder_unref(vb);
}

static void prv_context_new(const gchar *ip_address,
			    GUPnPDeviceProxy *proxy,
			    rsu_device_t *device,
			    rsu_device_context_t **context)
{
	const gchar *cm_type =
		"urn:schemas-upnp-org:service:ConnectionManager";
	const gchar *av_type =
		"urn:schemas-upnp-org:service:AVTransport";
	const gchar *rc_type =
		"urn:schemas-upnp-org:service:RenderingControl";
	rsu_device_context_t *ctx = g_new(rsu_device_context_t, 1);
	rsu_service_proxies_t *service_proxies = &ctx->service_proxies;

	ctx->ip_address = g_strdup(ip_address);
	ctx->device_proxy = proxy;
	ctx->device = device;
	g_object_ref(proxy);

	service_proxies->cm_proxy = (GUPnPServiceProxy *)
		gupnp_device_info_get_service((GUPnPDeviceInfo *) proxy,
					      cm_type);
	service_proxies->av_proxy = (GUPnPServiceProxy *)
		gupnp_device_info_get_service((GUPnPDeviceInfo *) proxy,
					      av_type);

	gupnp_service_proxy_set_subscribed(service_proxies->cm_proxy, TRUE);
	(void) gupnp_service_proxy_add_notify(service_proxies->cm_proxy,
					      "SinkProtocolInfo", G_TYPE_STRING,
					      prv_sink_change_cb,
					      device);

	gupnp_service_proxy_set_subscribed(service_proxies->av_proxy, TRUE);
	(void) gupnp_service_proxy_add_notify(service_proxies->av_proxy,
					      "LastChange", G_TYPE_STRING,
					      prv_last_change_cb,
					      device);
	service_proxies->rc_proxy = (GUPnPServiceProxy *)
		gupnp_device_info_get_service((GUPnPDeviceInfo *) proxy,
					      rc_type);

	*context = ctx;
}

void rsu_device_append_new_context(rsu_device_t *device,
				   const gchar *ip_address,
				   GUPnPDeviceProxy *proxy)
{
	rsu_device_context_t *context;

	prv_context_new(ip_address, proxy, device, &context);
	g_ptr_array_add(device->contexts, context);
}

void rsu_device_delete(void *device)
{
	unsigned int i;
	rsu_device_t *dev = device;

	if (dev) {
		for (i = 0; i < RSU_INTERFACE_INFO_MAX && dev->ids[i]; ++i)
			(void) g_dbus_connection_unregister_object(
				dev->connection,
				dev->ids[i]);
		g_ptr_array_unref(dev->contexts);
		g_free(dev->path);
		prv_props_free(&dev->props);
		g_free(dev);
	}
}

gboolean rsu_device_new(GDBusConnection *connection,
			GUPnPDeviceProxy *proxy,
			const gchar *ip_address,
			guint counter,
			rsu_interface_info_t *interface_info,
			void *user_data,
			rsu_device_t **device)
{
	rsu_device_t *dev = g_new0(rsu_device_t, 1);
	GString *new_path = NULL;
	unsigned int i;

	prv_props_init(&dev->props);
	dev->connection = connection;
	dev->contexts = g_ptr_array_new_with_free_func(prv_rsu_context_delete);

	rsu_device_append_new_context(dev, ip_address, proxy);

	new_path = g_string_new("");
	g_string_printf(new_path, "%s/%u", RSU_SERVER_PATH, counter);

	for (i = 0; i < RSU_INTERFACE_INFO_MAX; ++i) {
		dev->ids[i] = g_dbus_connection_register_object(
			connection,
			new_path->str,
			interface_info[i].interface,
			interface_info[i].vtable,
			user_data, NULL, NULL);

		if (!dev->ids[i])
			goto on_error;
	}

	dev->path = g_string_free(new_path, FALSE);

	*device = dev;
	return TRUE;

on_error:

	if (new_path)
		g_string_free(new_path, TRUE);

	rsu_device_delete(dev);

	return FALSE;
}

rsu_device_t *rsu_device_from_path(const gchar *path, GHashTable *device_list)
{
	GHashTableIter iter;
	gpointer value;
	rsu_device_t *device;
	rsu_device_t *retval = NULL;

	g_hash_table_iter_init(&iter, device_list);
	while (g_hash_table_iter_next(&iter, NULL, &value)) {
		device = value;
		if (!strcmp(device->path, path)) {
			retval = device;
			break;
		}
	}

	return retval;
}

rsu_device_context_t *rsu_device_get_context(rsu_device_t *device)
{
	rsu_device_context_t *context;
	unsigned int i;
	const char ip4_local_prefix[] = "127.0.0.";

	for (i = 0; i < device->contexts->len; ++i) {
		context = g_ptr_array_index(device->contexts, i);
		if (!strncmp(context->ip_address, ip4_local_prefix,
			     sizeof(ip4_local_prefix) - 1) ||
		    !strcmp(context->ip_address, "::1") ||
		    !strcmp(context->ip_address, "0:0:0:0:0:0:0:1"))
			break;
	}

	if (i == device->contexts->len)
		context = g_ptr_array_index(device->contexts, 0);

	return context;
}

static void prv_get_prop(rsu_async_cb_data_t *cb_data)
{
	rsu_task_get_prop_t *get_prop = &cb_data->task->ut.get_prop;
	GVariant *res = NULL;

	if (!strcmp(get_prop->interface_name, RSU_INTERFACE_SERVER)) {
		res = g_hash_table_lookup(cb_data->device->props.root_props,
					  get_prop->prop_name);
	} else if (!strcmp(get_prop->interface_name, RSU_INTERFACE_PLAYER)) {
		res = g_hash_table_lookup(cb_data->device->props.player_props,
					  get_prop->prop_name);
	} else if (!strcmp(get_prop->interface_name, "")) {
		res = g_hash_table_lookup(cb_data->device->props.root_props,
					  get_prop->prop_name);
		if (!res)
			res = g_hash_table_lookup(
				cb_data->device->props.player_props,
				get_prop->prop_name);
	} else {
		cb_data->error = g_error_new(RSU_ERROR,
					     RSU_ERROR_UNKNOWN_INTERFACE,
					     "Unknown Interface");
	}

	if (!res) {
		if (!cb_data->error)
			cb_data->error =
				g_error_new(RSU_ERROR,
					    RSU_ERROR_UNKNOWN_PROPERTY,
					    "Property not defined for object");
	} else {
		cb_data->result = g_variant_ref(res);
	}
}

static void prv_add_props(GHashTable *props, GVariantBuilder *vb)
{
	GHashTableIter iter;
	gpointer key;
	gpointer value;

	g_hash_table_iter_init(&iter, props);

	while (g_hash_table_iter_next(&iter, &key, &value))
		g_variant_builder_add(vb, "{sv}", (gchar *) key,
				      (GVariant *) value);
}

static void prv_get_props(rsu_async_cb_data_t *cb_data)
{
	rsu_task_get_props_t *get_props = &cb_data->task->ut.get_props;
	GVariantBuilder *vb;

	vb = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));

	if (!strcmp(get_props->interface_name, RSU_INTERFACE_SERVER)) {
		prv_add_props(cb_data->device->props.root_props, vb);
	} else if (!strcmp(get_props->interface_name, RSU_INTERFACE_PLAYER)) {
		prv_add_props(cb_data->device->props.player_props, vb);
	} else if (!strcmp(get_props->interface_name, "")) {
		prv_add_props(cb_data->device->props.root_props, vb);
		prv_add_props(cb_data->device->props.player_props, vb);
	} else {
		cb_data->error = g_error_new(RSU_ERROR,
					     RSU_ERROR_UNKNOWN_INTERFACE,
					     "Unknown Interface");
		goto on_error;
	}

	cb_data->result = g_variant_ref_sink(g_variant_builder_end(vb));

on_error:

	g_variant_builder_unref(vb);
}

static const gchar *prv_map_transport_state(const gchar *upnp_state)
{
	const gchar *retval;

	if (!strcmp(upnp_state, "PLAYING"))
		retval = "Playing";
	else if (!strcmp(upnp_state, "PAUSED_PLAYBACK") ||
		 !strcmp(upnp_state, "PAUSED_RECORDING"))
		retval = "Paused";
	else
		retval = "Stopped";

	return retval;
}

static gdouble prv_map_transport_speed(const gchar *upnp_speed)
{
	gdouble retval = 1;
	gchar **parts = NULL;
	gint num;
	gint dom;

	if (upnp_speed[0]) {
		parts = g_strsplit(upnp_speed, "/", 0);
		if (!parts[0])
			goto on_error;

		g_strstrip(parts[0]);
		num = atoi(parts[0]);

		if (parts[1]) {
			if (parts[2])
				goto on_error;

			g_strstrip(parts[1]);
			dom = atoi(parts[1]);
			if (dom == 0)
				goto on_error;

			retval = num / (gdouble) dom;
		} else {
			retval = num;
		}
	}

on_error:

	if (parts)
		g_strfreev(parts);

	return retval;
}

static void prv_add_actions(rsu_device_t *device, const gchar *actions)
{
	gchar **parts;
	unsigned int i = 0;
	GVariant *true_val;
	GVariant *false_val;
	gboolean play = FALSE;
	gboolean ppause = FALSE;
	gboolean seek = FALSE;
	gboolean next = FALSE;
	gboolean previous = FALSE;
	GVariant *val;

	parts = g_strsplit(actions, ",", 0);

	true_val = g_variant_ref_sink(g_variant_new_boolean(TRUE));
	false_val = g_variant_ref_sink(g_variant_new_boolean(FALSE));

	while (parts[i]) {
		g_strstrip(parts[i]);

		if (!strcmp(parts[i], "Play"))
			play = TRUE;
		else if (!strcmp(parts[i], "Pause"))
			ppause = TRUE;
		else if (!strcmp(parts[i], "Seek"))
			seek = TRUE;
		else if (!strcmp(parts[i], "Next"))
			next = TRUE;
		else if (!strcmp(parts[i], "Previous"))
			previous = TRUE;
		++i;
	}

	g_variant_ref(false_val);
	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_CAN_CONTROL, false_val);

	val = play ? true_val : false_val;
	g_variant_ref(val);
	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_CAN_PLAY, val);

	val = ppause ? true_val : false_val;
	g_variant_ref(val);
	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_CAN_PAUSE, val);

	val = seek ? true_val : false_val;
	g_variant_ref(val);
	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_CAN_SEEK, val);

	val = next ? true_val : false_val;
	g_variant_ref(val);
	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_CAN_NEXT, val);

	val = previous ? true_val : false_val;
	g_variant_ref(val);
	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_CAN_PREVIOUS, val);

	g_variant_unref(true_val);
	g_variant_unref(false_val);
	g_strfreev(parts);
}

static void prv_add_all_actions(rsu_device_t *device)
{
	GVariant *val;

	val = g_variant_ref_sink(g_variant_new_boolean(TRUE));
	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_CAN_PLAY, val);
	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_CAN_PAUSE, g_variant_ref(val));
	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_CAN_SEEK, g_variant_ref(val));
	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_CAN_NEXT, g_variant_ref(val));
	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_CAN_PREVIOUS,
			    g_variant_ref(val));
	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_CAN_CONTROL,
			    g_variant_ref(val));
}

static gint64 prv_duration_to_int64(const gchar *duration)
{
	gchar **parts;
	unsigned int i = 0;
	unsigned int count;
	gint64 pos = 0;

	parts = g_strsplit(duration, ":", 0);
	for (count = 0; parts[count]; ++count)
		;

	if (count != 3)
		goto on_error;

	/* TODO: This does not handle fractional seconds */

	i = 1;
	do {
		--count;
		g_strstrip(parts[count]);
		pos += atoi(parts[count]) * i;
		i *= 60;
	} while (count > 0);

	pos *= 1000000;

on_error:

	g_strfreev(parts);

	return pos;
}

static gchar *prv_int64_to_duration(gint64 micro_seconds)
{
	GString *retval;
	unsigned int seconds;

	if (micro_seconds < 0) {
		retval = g_string_new("-");
		micro_seconds = -micro_seconds;
	} else {
		retval = g_string_new("");
	}

	/* TODO: This does not handle fractional seconds */

	seconds = micro_seconds / 1000000;
	g_string_append_printf(retval, "%02u:%02u:%02u",
			       seconds / 3600,
			       (seconds / 60) % 60,
			       seconds % 60);

	return g_string_free(retval, FALSE);
}

static void prv_add_reltime(rsu_device_t *device, const gchar *reltime)
{
	GVariant *val;
	gint64 pos = prv_duration_to_int64(reltime);

	val = g_variant_ref_sink(g_variant_new_int64(pos));
	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_POSITION,
			    val);
}

static void prv_found_item(GUPnPDIDLLiteParser *parser,
			   GUPnPDIDLLiteObject *object,
			   gpointer user_data)
{
	GVariantBuilder *vb = user_data;
	gchar *track_id;
	int track_number = gupnp_didl_lite_object_get_track_number(object);
	GVariant *value;
	const gchar *str_value;
	GVariantBuilder *artists_vb;
	GVariantBuilder *album_artists_vb;
	GList *artists;
	GList *head;
	const gchar *artist_name;
	const gchar *artist_role;

	track_id = g_strdup_printf(RSU_OBJECT"/track/%u",
				   track_number != -1 ? track_number : 0);

	value = g_variant_new_string(track_id);
	g_variant_builder_add(vb, "{sv}", "mpris:trackid", value);
	g_free(track_id);

	if (track_number != -1) {
		value = g_variant_new_int32(track_number);
		g_variant_builder_add(vb, "{sv}", "mpris:trackNumber", value);
	}

	str_value = gupnp_didl_lite_object_get_title(object);
	if (str_value) {
		value = g_variant_new_string(str_value);
		g_variant_builder_add(vb, "{sv}", "xesam:title", value);
	}

	str_value = gupnp_didl_lite_object_get_album_art(object);
	if (str_value) {
		value = g_variant_new_string(str_value);
		g_variant_builder_add(vb, "{sv}", "mpris:artUrl", value);
	}

	str_value = gupnp_didl_lite_object_get_album(object);
	if (str_value) {
		value = g_variant_new_string(str_value);
		g_variant_builder_add(vb, "{sv}", "xesam:album", value);
	}

	str_value = gupnp_didl_lite_object_get_genre(object);
	if (str_value) {
		value = g_variant_new_string(str_value);
		g_variant_builder_add(vb, "{sv}", "xesam:genre", value);
	}

	head = artists = gupnp_didl_lite_object_get_artists(object);
	if (artists) {
		artists_vb = g_variant_builder_new(G_VARIANT_TYPE("as"));
		album_artists_vb = g_variant_builder_new(G_VARIANT_TYPE("as"));
		do {
			artist_name =
				gupnp_didl_lite_contributor_get_name(
					artists->data);
			artist_role = gupnp_didl_lite_contributor_get_role(
				artists->data);
			if (!artist_role)
				g_variant_builder_add(artists_vb, "s",
						      artist_name);
			else if (!strcmp(artist_role, "AlbumArtist"))
				g_variant_builder_add(album_artists_vb, "s",
						      artist_name);
			g_object_unref(artists->data);
			artists = g_list_next(artists);
		} while (artists);
		g_list_free(head);
		value = g_variant_builder_end(artists_vb);
		g_variant_builder_add(vb, "{sv}", "xesam:artist", value);
		value = g_variant_builder_end(album_artists_vb);
		g_variant_builder_add(vb, "{sv}", "xesam:albumArtist", value);
		g_variant_builder_unref(artists_vb);
		g_variant_builder_unref(album_artists_vb);
	}
}

static void prv_add_track_meta_data(rsu_device_t *device, const gchar *metadata,
				    const gchar *duration, const gchar *uri)
{
	gchar *didl = g_strdup_printf("<DIDL-Lite>%s</DIDL-Lite>", metadata);
	GUPnPDIDLLiteParser *parser = NULL;
	GVariantBuilder *vb;
	GError *upnp_error = NULL;
	GVariant *val;
	gint error_code;

	parser = gupnp_didl_lite_parser_new();

	vb = g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));

	if (duration) {
		val = g_variant_new_int64(prv_duration_to_int64(duration));
		g_variant_builder_add(vb, "{sv}", "mpris:length", val);
	}

	if (uri) {
		val = g_variant_new_string(uri);
		g_variant_builder_add(vb, "{sv}", "xesam:url", val);
	}

	g_signal_connect(parser, "object-available" ,
			 G_CALLBACK(prv_found_item), vb);

	if (!gupnp_didl_lite_parser_parse_didl(parser, didl, &upnp_error)) {
		error_code = upnp_error->code;
		g_error_free(upnp_error);
		if (error_code != GUPNP_XML_ERROR_EMPTY_NODE)
			goto on_error;
	}

	g_hash_table_insert(device->props.player_props,
			    RSU_INTERFACE_PROP_METADATA,
			    g_variant_ref_sink(g_variant_builder_end(vb)));

on_error:

	if (parser)
		g_object_unref(parser);

	g_variant_builder_unref(vb);
	g_free(didl);
}

static void prv_last_change_cb(GUPnPServiceProxy *proxy,
			       const char *variable,
			       GValue *value,
			       gpointer user_data)
{
	GUPnPLastChangeParser *parser;
	rsu_device_t *device = user_data;
	gchar *meta_data = NULL;
	gchar *actions = NULL;
	gchar *play_speed = NULL;
	gchar *state = NULL;
	gchar *duration = NULL;
	gchar *uri = NULL;
	GVariant *val;

	parser = gupnp_last_change_parser_new();

	if (!gupnp_last_change_parser_parse_last_change(
		    parser, 0,
		    g_value_get_string(value),
		    NULL,
		    "CurrentTrackMetaData", G_TYPE_STRING, &meta_data,
		    "CurrentTransportActions", G_TYPE_STRING, &actions,
		    "TransportPlaySpeed", G_TYPE_STRING, &play_speed,
		    "TransportState", G_TYPE_STRING, &state,
		    "CurrentTrackDuration", G_TYPE_STRING, &duration,
		    "CurrentTrackURI" , G_TYPE_STRING, &uri,
		    NULL))
		goto on_error;

	if (meta_data) {
		prv_add_track_meta_data(device, meta_data, duration, uri);
		g_free(meta_data);
	} else {
		if (duration) {
			val = g_variant_new_int64(prv_duration_to_int64(
							  duration));
			val = g_variant_ref_sink(val);
			prv_merge_meta_data(device, "mpris:length", val);
			g_variant_unref(val);
		}

		if (uri) {
			val = g_variant_ref_sink(g_variant_new_string(uri));
			prv_merge_meta_data(device, "xesam:url", val);
			g_variant_unref(val);
		}
	}

	g_free(duration);
	g_free(uri);

	if (actions) {
		prv_add_actions(device, actions);
		g_free(actions);
	}

	if (play_speed) {
		val = g_variant_ref_sink(
			g_variant_new_double(
				prv_map_transport_speed(play_speed)));
		g_hash_table_insert(device->props.player_props,
				    RSU_INTERFACE_PROP_RATE, val);
		g_free(play_speed);
	}

	if (state) {
		val = g_variant_ref_sink(
			g_variant_new_string(
				prv_map_transport_state(state)));
		g_hash_table_insert(device->props.player_props,
				    RSU_INTERFACE_PROP_PLAYBACK_STATUS,
				    val);
		g_free(state);
	}

on_error:

	g_object_unref(parser);
}


static void prv_as_prop_from_hash_table(const gchar *prop_name,
					GHashTable *values, GHashTable *props)
{
	GVariantBuilder vb;
	GHashTableIter iter;
	gpointer key;
	GVariant *val;

	g_variant_builder_init(&vb, G_VARIANT_TYPE("as"));
	g_hash_table_iter_init(&iter, values);

	while (g_hash_table_iter_next(&iter, &key, NULL))
		g_variant_builder_add(&vb, "s", key);

	val = g_variant_ref_sink(g_variant_builder_end(&vb));
	g_hash_table_insert(props, (gchar *) prop_name, val);
}

static void prv_process_protocol_info(rsu_device_t *device,
				      const gchar *protocol_info)
{
	GVariant *val;
	gchar **entries;
	gchar **type_info;
	unsigned int i;
	GHashTable *protocols;
	GHashTable *types;
	const char http_prefix[] = "http-";

	protocols = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
					  NULL);
	types = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	val = g_variant_ref_sink(g_variant_new_string(protocol_info));
	g_hash_table_insert(device->props.root_props,
			    RSU_INTERFACE_PROP_PROTOCOL_INFO,
			    val);

	entries = g_strsplit(protocol_info, ",", 0);

	for (i = 0; entries[i]; ++i) {
		type_info = g_strsplit(entries[i], ":", 0);

		if (type_info[0] && type_info[1] && type_info[2]) {
			if (!g_ascii_strncasecmp(http_prefix, type_info[0],
						 sizeof(http_prefix) - 1)) {
				type_info[0][sizeof(http_prefix) - 2] = 0;
			}

			g_hash_table_insert(protocols,
					    g_ascii_strdown(type_info[0], -1),
					    NULL);
			g_hash_table_insert(types,
					    g_ascii_strdown(type_info[2], -1),
					    NULL);
		}

		g_strfreev(type_info);
	}

	g_strfreev(entries);

	prv_as_prop_from_hash_table(RSU_INTERFACE_PROP_SUPPORTED_URIS,
				    protocols,
				    device->props.root_props);

	prv_as_prop_from_hash_table(RSU_INTERFACE_PROP_SUPPORTED_MIME,
				    types,
				    device->props.root_props);

	g_hash_table_unref(types);
	g_hash_table_unref(protocols);
}

static void prv_sink_change_cb(GUPnPServiceProxy *proxy,
			       const char *variable,
			       GValue *value,
			       gpointer user_data)
{
	rsu_device_t *device = user_data;
	const gchar *sink;

	sink = g_value_get_string(value);

	if (sink)
		prv_process_protocol_info(device, sink);
}

static void prv_get_position_info_cb(GUPnPServiceProxy *proxy,
				     GUPnPServiceProxyAction *action,
				     gpointer user_data)
{
	gchar *rel_pos = NULL;
	rsu_async_cb_data_t *cb_data = user_data;
	GError *upnp_error = NULL;
	rsu_device_data_t *device_data = cb_data->private;

	if (!gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					    &upnp_error,
					    "RelTime",
					    G_TYPE_STRING, &rel_pos, NULL)) {
		cb_data->error = g_error_new(RSU_ERROR,
					     RSU_ERROR_OPERATION_FAILED,
					     "GetPositionInfo operation "
					     "failed: %s", upnp_error->message);
		g_error_free(upnp_error);

		goto on_error;
	}

	g_strstrip(rel_pos);
	prv_add_reltime(cb_data->device, rel_pos);
	g_free(rel_pos);

on_error:

	device_data->local_cb(cb_data);
}

static void prv_get_position_info(GCancellable *cancellable,
				  rsu_async_cb_data_t *cb_data)
{
	rsu_device_context_t *context;

	context = rsu_device_get_context(cb_data->device);

	cb_data->cancel_id =
		g_cancellable_connect(cancellable,
				      G_CALLBACK(rsu_async_task_cancelled),
				      cb_data, NULL);
	cb_data->cancellable = cancellable;
	cb_data->proxy = context->service_proxies.av_proxy;
	cb_data->action =
		gupnp_service_proxy_begin_action(cb_data->proxy,
						 "GetPositionInfo",
						 prv_get_position_info_cb,
						 cb_data,
						 "InstanceID", G_TYPE_INT, 0,
						 NULL);
}

static void prv_props_update(rsu_device_t *device, rsu_task_t *task)
{
	GVariant *val;
	GUPnPDeviceInfo *info;
	rsu_device_context_t *context;
	rsu_props_t *props = &device->props;
	gchar *friendly_name;

	context = rsu_device_get_context(device);

	val = g_variant_ref_sink(g_variant_new_boolean(FALSE));
	g_hash_table_insert(props->root_props, RSU_INTERFACE_PROP_CAN_QUIT,
			    val);

	g_hash_table_insert(props->root_props, RSU_INTERFACE_PROP_CAN_RAISE,
			    g_variant_ref(val));

	g_hash_table_insert(props->root_props,
			    RSU_INTERFACE_PROP_CAN_SET_FULLSCREEN,
			    g_variant_ref(val));

	g_hash_table_insert(props->root_props,
			    RSU_INTERFACE_PROP_HAS_TRACK_LIST,
			    g_variant_ref(val));

	/* TODO:  The following three properties require information to
	   be read out of the service file to be properly implemented.
	   The problem is that GUPnP does not currently allow you to
	   cancel requests to access the service file */

	val = g_variant_ref_sink(g_variant_new_double(1.0));
	g_hash_table_insert(props->player_props,
			    RSU_INTERFACE_PROP_MINIMUM_RATE, val);
	g_hash_table_insert(props->player_props,
			    RSU_INTERFACE_PROP_MAXIMUM_RATE,
			    g_variant_ref(val));
	g_hash_table_insert(props->player_props,
			    RSU_INTERFACE_PROP_VOLUME,
			    g_variant_ref(val));

	info = (GUPnPDeviceInfo *) context->device_proxy;
	friendly_name = gupnp_device_info_get_friendly_name(info);
	val = g_variant_ref_sink(g_variant_new_string(friendly_name));
	g_free(friendly_name);
	g_hash_table_insert(props->root_props, RSU_INTERFACE_PROP_IDENTITY,
			    val);
	prv_add_all_actions(device);
	device->props.synced = TRUE;
}

static void prv_complete_get_prop(rsu_async_cb_data_t *cb_data)
{
	prv_get_prop(cb_data);
	(void) g_idle_add(rsu_async_complete_task, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);
}

static void prv_complete_get_props(rsu_async_cb_data_t *cb_data)
{
	prv_get_props(cb_data);
	(void) g_idle_add(rsu_async_complete_task, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);
}

void rsu_device_get_prop(rsu_device_t *device, rsu_task_t *task,
			 GCancellable *cancellable,
			 rsu_upnp_task_complete_t cb,
			 void *user_data)
{
	rsu_async_cb_data_t *cb_data;
	rsu_task_get_prop_t *get_prop = &task->ut.get_prop;
	rsu_device_data_t *device_cb_data;

	/* Need to check to see if the property is RSU_INTERFACE_PROP_POSITION.
	   If it is we need to call GetPositionInfo.  This value is not evented.
	   Otherwise we can just update the value straight away. */

	if ((!strcmp(get_prop->interface_name, RSU_INTERFACE_PLAYER) ||
	     !strcmp(get_prop->interface_name, "")) &&
	    (!strcmp(task->ut.get_prop.prop_name, RSU_INTERFACE_PROP_POSITION))) {
		/* Need to read the current position.  This property is not
		   evented */

		device_cb_data = g_new(rsu_device_data_t, 1);
		device_cb_data->local_cb = prv_complete_get_prop;

		cb_data = rsu_async_cb_data_new(task, cb, user_data,
						device_cb_data, g_free,
						device);

		prv_get_position_info(cancellable, cb_data);
	} else {
		cb_data = rsu_async_cb_data_new(task, cb, user_data, NULL, NULL,
						device);

		if (!device->props.synced)
			prv_props_update(device, task);

		prv_get_prop(cb_data);
		(void) g_idle_add(rsu_async_complete_task, cb_data);
	}
}

void rsu_device_get_all_props(rsu_device_t *device, rsu_task_t *task,
			      GCancellable *cancellable,
			      rsu_upnp_task_complete_t cb,
			      void *user_data)
{
	rsu_async_cb_data_t *cb_data;
	rsu_task_get_props_t *get_props = &task->ut.get_props;
	rsu_device_data_t *device_cb_data;

	if (!device->props.synced)
		prv_props_update(device, task);

	if ((!strcmp(get_props->interface_name, RSU_INTERFACE_PLAYER) ||
	     !strcmp(get_props->interface_name, ""))) {

		/* Need to read the current position.  This property is not
		   evented */

		device_cb_data = g_new(rsu_device_data_t, 1);
		device_cb_data->local_cb = prv_complete_get_props;

		cb_data = rsu_async_cb_data_new(task, cb, user_data,
						device_cb_data, NULL,
						device);

		prv_get_position_info(cancellable, cb_data);
	} else {
		cb_data = rsu_async_cb_data_new(task, cb, user_data, NULL, NULL,
					device);

		prv_get_props(cb_data);
		(void) g_idle_add(rsu_async_complete_task, cb_data);
	}
}

static void prv_simple_call_cb(GUPnPServiceProxy *proxy,
			       GUPnPServiceProxyAction *action,
			       gpointer user_data)
{
	rsu_async_cb_data_t *cb_data = user_data;
	GError *upnp_error = NULL;

	if (!gupnp_service_proxy_end_action(cb_data->proxy, cb_data->action,
					    &upnp_error, NULL)) {
		cb_data->error = g_error_new(RSU_ERROR,
					     RSU_ERROR_OPERATION_FAILED,
					     "Operation "
					     "failed: %s", upnp_error->message);
		g_error_free(upnp_error);
	}

	(void) g_idle_add(rsu_async_complete_task, cb_data);
	g_cancellable_disconnect(cb_data->cancellable, cb_data->cancel_id);
}

void rsu_device_play(rsu_device_t *device, rsu_task_t *task,
		     GCancellable *cancellable,
		     rsu_upnp_task_complete_t cb,
		     void *user_data)
{
	rsu_device_context_t *context;
	rsu_async_cb_data_t *cb_data;

	context = rsu_device_get_context(device);
	cb_data = rsu_async_cb_data_new(task, cb, user_data, NULL, NULL,
					device);

	cb_data->cancel_id =
		g_cancellable_connect(cancellable,
				      G_CALLBACK(rsu_async_task_cancelled),
				      cb_data, NULL);
	cb_data->cancellable = cancellable;
	cb_data->proxy = context->service_proxies.av_proxy;
	cb_data->action =
		gupnp_service_proxy_begin_action(cb_data->proxy,
						 "Play",
						 prv_simple_call_cb,
						 cb_data,
						 "InstanceID", G_TYPE_INT, 0,
						 "Speed", G_TYPE_STRING, "1",
						 NULL);
}

void rsu_device_play_pause(rsu_device_t *device, rsu_task_t *task,
			   GCancellable *cancellable,
			   rsu_upnp_task_complete_t cb,
			   void *user_data)
{
	GVariant *state;

	state = g_hash_table_lookup(device->props.player_props,
				    RSU_INTERFACE_PROP_PLAYBACK_STATUS);

	if (state && !strcmp(g_variant_get_string(state, NULL), "Playing"))
		rsu_device_pause(device, task, cancellable, cb, user_data);
	else
		rsu_device_play(device, task, cancellable, cb, user_data);
}

static void prv_simple_command(rsu_device_t *device, rsu_task_t *task,
			       const gchar *command_name,
			       GCancellable *cancellable,
			       rsu_upnp_task_complete_t cb,
			       void *user_data)
{
	rsu_device_context_t *context;
	rsu_async_cb_data_t *cb_data;

	context = rsu_device_get_context(device);
	cb_data = rsu_async_cb_data_new(task, cb, user_data, NULL, NULL,
					device);

	cb_data->cancel_id =
		g_cancellable_connect(cancellable,
				      G_CALLBACK(rsu_async_task_cancelled),
				      cb_data, NULL);
	cb_data->cancellable = cancellable;
	cb_data->proxy = context->service_proxies.av_proxy;
	cb_data->action =
		gupnp_service_proxy_begin_action(cb_data->proxy,
						 command_name,
						 prv_simple_call_cb,
						 cb_data,
						 "InstanceID", G_TYPE_INT, 0,
						 NULL);
}
void rsu_device_pause(rsu_device_t *device, rsu_task_t *task,
		      GCancellable *cancellable,
		      rsu_upnp_task_complete_t cb,
		      void *user_data)
{
	prv_simple_command(device, task, "Pause", cancellable,
			   cb, user_data);
}

void rsu_device_stop(rsu_device_t *device, rsu_task_t *task,
		     GCancellable *cancellable,
		     rsu_upnp_task_complete_t cb,
		     void *user_data)
{
	prv_simple_command(device, task, "Stop", cancellable,
			   cb, user_data);
}

void rsu_device_next(rsu_device_t *device, rsu_task_t *task,
		     GCancellable *cancellable,
		     rsu_upnp_task_complete_t cb,
		     void *user_data)
{
	prv_simple_command(device, task, "Next", cancellable,
			   cb, user_data);
}

void rsu_device_previous(rsu_device_t *device, rsu_task_t *task,
			 GCancellable *cancellable,
			 rsu_upnp_task_complete_t cb,
			 void *user_data)
{
	prv_simple_command(device, task, "Previous", cancellable,
			   cb, user_data);
}

void rsu_device_open_uri(rsu_device_t *device, rsu_task_t *task,
			 GCancellable *cancellable,
			 rsu_upnp_task_complete_t cb,
			 void *user_data)
{
	rsu_device_context_t *context;
	rsu_async_cb_data_t *cb_data;
	rsu_task_open_uri_t *open_uri_data = &task->ut.open_uri;

	context = rsu_device_get_context(device);
	cb_data = rsu_async_cb_data_new(task, cb, user_data, NULL, NULL,
					device);

	cb_data->cancel_id =
		g_cancellable_connect(cancellable,
				      G_CALLBACK(rsu_async_task_cancelled),
				      cb_data, NULL);
	cb_data->cancellable = cancellable;
	cb_data->proxy = context->service_proxies.av_proxy;
	cb_data->action =
		gupnp_service_proxy_begin_action(cb_data->proxy,
						 "SetAVTransportURI",
						 prv_simple_call_cb,
						 cb_data,
						 "InstanceID", G_TYPE_INT, 0,
						 "CurrentURI", G_TYPE_STRING,
						 open_uri_data->uri,
						 "CurrentURIMetaData",
						 G_TYPE_STRING, "",
						 NULL);
}

static void prv_device_set_position(rsu_device_t *device, rsu_task_t *task,
				    const gchar *pos_type,
				    GCancellable *cancellable,
				    rsu_upnp_task_complete_t cb,
				    void *user_data)
{
	rsu_device_context_t *context;
	rsu_async_cb_data_t *cb_data;
	rsu_task_seek_t *seek_data = &task->ut.seek;
	gchar *position;

	context = rsu_device_get_context(device);
	cb_data = rsu_async_cb_data_new(task, cb, user_data, NULL, NULL,
					device);

	position = prv_int64_to_duration(seek_data->position);

	cb_data->cancel_id =
		g_cancellable_connect(cancellable,
				      G_CALLBACK(rsu_async_task_cancelled),
				      cb_data, NULL);
	cb_data->cancellable = cancellable;
	cb_data->proxy = context->service_proxies.av_proxy;
	cb_data->action =
		gupnp_service_proxy_begin_action(cb_data->proxy,
						 "Seek",
						 prv_simple_call_cb,
						 cb_data,
						 "InstanceID", G_TYPE_INT, 0,
						 "Unit", G_TYPE_STRING,
						 pos_type,
						 "Target",
						 G_TYPE_STRING, position,
						 NULL);

	g_free(position);
}

void rsu_device_seek(rsu_device_t *device, rsu_task_t *task,
		     GCancellable *cancellable,
		     rsu_upnp_task_complete_t cb,
		     void *user_data)
{
	prv_device_set_position(device, task,  "REL_TIME",
				cancellable, cb, user_data);
}

void rsu_device_set_position(rsu_device_t *device, rsu_task_t *task,
			     GCancellable *cancellable,
			     rsu_upnp_task_complete_t cb,
			     void *user_data)
{
	prv_device_set_position(device, task,  "ABS_TIME",
				cancellable, cb, user_data);
}

void rsu_device_host_uri(rsu_device_t *device, rsu_task_t *task,
			 rsu_host_service_t *host_service,
			 GCancellable *cancellable,
			 rsu_upnp_task_complete_t cb,
			 void *user_data)
{
	rsu_device_context_t *context;
	rsu_async_cb_data_t *cb_data;
	rsu_task_host_uri_t *host_uri = &task->ut.host_uri;
	gchar *url;
	GError *error = NULL;

	context = rsu_device_get_context(device);
	url = rsu_host_service_add(host_service, context->ip_address,
				   host_uri->client, host_uri->uri,
				   &error);

	cb_data = rsu_async_cb_data_new(task, cb, user_data, NULL, NULL,
					device);
	if (url) {
		cb_data->result = g_variant_ref_sink(g_variant_new_string(url));
		g_free(url);
	} else {
		cb_data->error  = error;
	}

	(void) g_idle_add(rsu_async_complete_task, cb_data);
}

void rsu_device_remove_uri(rsu_device_t *device, rsu_task_t *task,
			   rsu_host_service_t *host_service,
			   GCancellable *cancellable,
			   rsu_upnp_task_complete_t cb,
			   void *user_data)
{
	rsu_device_context_t *context;
	rsu_async_cb_data_t *cb_data;
	rsu_task_host_uri_t *host_uri = &task->ut.host_uri;

	context = rsu_device_get_context(device);
	cb_data = rsu_async_cb_data_new(task, cb, user_data, NULL, NULL,
					device);

	if (!rsu_host_service_remove(host_service, context->ip_address,
				     host_uri->client, host_uri->uri)) {
		cb_data->error = g_error_new(RSU_ERROR,
					     RSU_ERROR_OBJECT_NOT_FOUND,
					     "File not hosted for specified "
					     " device");
	}

	(void) g_idle_add(rsu_async_complete_task, cb_data);
}
