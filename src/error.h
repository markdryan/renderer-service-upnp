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

#ifndef RSU_ERROR_H__
#define RSU_ERROR_H__

#include <glib.h>

enum rsu_error_t_ {
	RSU_ERROR_BAD_PATH,
	RSU_ERROR_OBJECT_NOT_FOUND,
	RSU_ERROR_BAD_QUERY,
	RSU_ERROR_OPERATION_FAILED,
	RSU_ERROR_BAD_RESULT,
	RSU_ERROR_UNKNOWN_INTERFACE,
	RSU_ERROR_UNKNOWN_PROPERTY,
	RSU_ERROR_DEVICE_NOT_FOUND,
	RSU_ERROR_DIED,
	RSU_ERROR_CANCELLED,
	RSU_ERROR_NOT_SUPPORTED,
	RSU_ERROR_LOST_OBJECT,
	RSU_ERROR_BAD_MIME,
	RSU_ERROR_HOST_FAILED
};
typedef enum rsu_error_t_ rsu_error_t;

#define RSU_ERROR (rsu_error_quark())
GQuark rsu_error_quark(void);

#endif
