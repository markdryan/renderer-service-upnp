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

#include "config.h"

#include <glib.h>
#include <gio/gio.h>

#include "error.h"


static const GDBusErrorEntry g_error_entries[] = {
	{ RSU_ERROR_BAD_PATH, RSU_SERVICE".BadPath" },
	{ RSU_ERROR_OBJECT_NOT_FOUND, RSU_SERVICE".ObjectNotFound" },
	{ RSU_ERROR_BAD_QUERY, RSU_SERVICE".BadQuery" },
	{ RSU_ERROR_OPERATION_FAILED, RSU_SERVICE".OperationFailed" },
	{ RSU_ERROR_BAD_RESULT, RSU_SERVICE".BadResult" },
	{ RSU_ERROR_UNKNOWN_INTERFACE, RSU_SERVICE".UnknownInterface" },
	{ RSU_ERROR_UNKNOWN_PROPERTY, RSU_SERVICE".UnknownProperty" },
	{ RSU_ERROR_DEVICE_NOT_FOUND, RSU_SERVICE".DeviceNotFound" },
	{ RSU_ERROR_DIED, RSU_SERVICE".Died" },
	{ RSU_ERROR_CANCELLED, RSU_SERVICE".Cancelled" },
	{ RSU_ERROR_NOT_SUPPORTED, RSU_SERVICE".NotSupported" },
	{ RSU_ERROR_LOST_OBJECT, RSU_SERVICE".LostObject" },
	{ RSU_ERROR_BAD_MIME, RSU_SERVICE".BadMime" },
	{ RSU_ERROR_HOST_FAILED, RSU_SERVICE".HostFailed" }
};

GQuark rsu_error_quark(void)
{
	static gsize quark = 0;
	g_dbus_error_register_error_domain("mc-error-quark", &quark,
					   g_error_entries,
					   sizeof(g_error_entries) /
					   sizeof(const GDBusErrorEntry));
	return (GQuark) quark;
}
