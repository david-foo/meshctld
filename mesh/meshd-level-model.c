#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <wordexp.h>

#include <inttypes.h>
#include <ctype.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dbus/dbus.h>
#include <string.h>

#include "gdbus/gdbus.h"
#include "src/shared/mainloop.h"
#include "src/shared/shell.h"
#include "mesh/dbus-server.h"
#include "mesh/meshd-model.h"
#include "mesh/meshd-level-model.h"


static DBusMessage *exec_level_get(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *reply;
	DBusMessageIter array, dict, entry, variant;
	int error;
	int i, array_len, int_value;
	int variant_type;
	const char* key;
	uint16_t addr = 0;

	dbus_message_iter_init(msg, &array);

	if(strcmp(dbus_message_iter_get_signature(&array), "a{sv}")) {
		return meshd_error_invalid_args(msg);
	}
	array_len = dbus_message_iter_get_element_count(&array);
	dbus_message_iter_recurse(&array, &dict);

	for(i = 0; i < array_len; i++) {
		dbus_message_iter_recurse(&dict, &entry);
		dbus_message_iter_get_basic(&entry, &key);
		dbus_message_iter_next(&entry);
		dbus_message_iter_recurse(&entry, &variant);
		variant_type = dbus_message_iter_get_arg_type(&variant);
		if(variant_type == DBUS_TYPE_UINT16 || variant_type == DBUS_TYPE_UINT32 ||
				variant_type == DBUS_TYPE_INT16 || variant_type == DBUS_TYPE_INT32) {
			dbus_message_iter_get_basic(&variant, &int_value);
			if(!strcmp(key, "addr")) {
				addr = int_value & 0xFFFF;
			}
		}
		dbus_message_iter_next(&dict);
	}

	bt_shell_printf("[dbus][%s][%s] %s.%s"
		"(addr = %4.4x)\n",
		dbus_message_get_destination(msg),
		dbus_message_get_path(msg),
		dbus_message_get_interface(msg),
		dbus_message_get_member(msg),
		addr);

	error = bt_shell_manual_input_fmt("level.get %d",
						addr);

	reply = dbus_message_new_method_return(msg);

	return reply;
}

static DBusMessage *exec_level_set(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *reply;
	DBusMessageIter array, dict, entry, variant;
	int error;
	int i, array_len, int_value;
	int variant_type;
	const char* key;
	uint16_t addr = 0;
	int16_t state = 0;
	uint16_t time = 0, delay = 0;

	dbus_message_iter_init(msg, &array);

	if(strcmp(dbus_message_iter_get_signature(&array), "a{sv}")) {
		return meshd_error_invalid_args(msg);
	}
	array_len = dbus_message_iter_get_element_count(&array);
	dbus_message_iter_recurse(&array, &dict);

	for(i = 0; i < array_len; i++) {
		dbus_message_iter_recurse(&dict, &entry);
		dbus_message_iter_get_basic(&entry, &key);
		dbus_message_iter_next(&entry);
		dbus_message_iter_recurse(&entry, &variant);
		variant_type = dbus_message_iter_get_arg_type(&variant);
		if(variant_type == DBUS_TYPE_UINT16 || variant_type == DBUS_TYPE_UINT32 ||
				variant_type == DBUS_TYPE_INT16 || variant_type == DBUS_TYPE_INT32) {
			dbus_message_iter_get_basic(&variant, &int_value);
			if(!strcmp(key, "addr")) {
				addr = int_value & 0xFFFF;
			} else if(!strcmp(key, "state")) {
				state = int_value & 0xFFFF;
			} else if(!strcmp(key, "time")) {
				time = int_value & 0xFFFF;
			} else if(!strcmp(key, "delay")) {
				delay = int_value & 0xFFFF;
			}
		}
		dbus_message_iter_next(&dict);
	}

	bt_shell_printf("[dbus][%s][%s] %s.%s"
		"(addr = %4.4x, state = %d, time = %d, delay = %d)\n",
		dbus_message_get_destination(msg),
		dbus_message_get_path(msg),
		dbus_message_get_interface(msg),
		dbus_message_get_member(msg),
		addr, state, time ,delay);

	if (array_len > 2) {
		error = bt_shell_manual_input_fmt("level.set %d %d %d %d",
						addr, state, time, delay);
	}
	else {
		error = bt_shell_manual_input_fmt("level.set %d %d",
						addr, state);
	}

	reply = dbus_message_new_method_return(msg);

	return reply;
}

static DBusMessage *exec_delta_set(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *reply;
	DBusMessageIter array, dict, entry, variant;
	int error;
	int i, array_len, int_value;
	int variant_type;
	const char* key;
	uint16_t addr = 0;
	int16_t state = 0;
	uint16_t time = 0, delay = 0;

	dbus_message_iter_init(msg, &array);

	if(strcmp(dbus_message_iter_get_signature(&array), "a{sv}")) {
		return meshd_error_invalid_args(msg);
	}
	array_len = dbus_message_iter_get_element_count(&array);
	dbus_message_iter_recurse(&array, &dict);

	for(i = 0; i < array_len; i++) {
		dbus_message_iter_recurse(&dict, &entry);
		dbus_message_iter_get_basic(&entry, &key);
		dbus_message_iter_next(&entry);
		dbus_message_iter_recurse(&entry, &variant);
		variant_type = dbus_message_iter_get_arg_type(&variant);
		if(variant_type == DBUS_TYPE_UINT16 || variant_type == DBUS_TYPE_UINT32 ||
				variant_type == DBUS_TYPE_INT16 || variant_type == DBUS_TYPE_INT32) {
			dbus_message_iter_get_basic(&variant, &int_value);
			if(!strcmp(key, "addr")) {
				addr = int_value & 0xFFFF;
			} else if(!strcmp(key, "state")) {
				state = int_value & 0xFFFF;
			} else if(!strcmp(key, "time")) {
				time = int_value & 0xFFFF;
			} else if(!strcmp(key, "delay")) {
				delay = int_value & 0xFFFF;
			}
		}
		dbus_message_iter_next(&dict);
	}

	bt_shell_printf("[dbus][%s][%s] %s.%s"
		"(addr = %4.4x, state = %d, time = %d, delay = %d)\n",
		dbus_message_get_destination(msg),
		dbus_message_get_path(msg),
		dbus_message_get_interface(msg),
		dbus_message_get_member(msg),
		addr, state, time ,delay);

	if (array_len > 2) {
		error = bt_shell_manual_input_fmt("level.delta %d %d %d %d",
						addr, state, time, delay);
	}
	else {
		error = bt_shell_manual_input_fmt("level.delta %d %d",
						addr, state);
	}

	reply = dbus_message_new_method_return(msg);

	return reply;
}

static DBusMessage *exec_move_set(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *reply;
	DBusMessageIter array, dict, entry, variant;
	int error;
	int i, array_len, int_value;
	int variant_type;
	const char* key;
	uint16_t addr = 0;
	int16_t state = 0;
	uint16_t time = 0, delay = 0;

	dbus_message_iter_init(msg, &array);

	if(strcmp(dbus_message_iter_get_signature(&array), "a{sv}")) {
		return meshd_error_invalid_args(msg);
	}
	array_len = dbus_message_iter_get_element_count(&array);
	dbus_message_iter_recurse(&array, &dict);

	for(i = 0; i < array_len; i++) {
		dbus_message_iter_recurse(&dict, &entry);
		dbus_message_iter_get_basic(&entry, &key);
		dbus_message_iter_next(&entry);
		dbus_message_iter_recurse(&entry, &variant);
		variant_type = dbus_message_iter_get_arg_type(&variant);
		if(variant_type == DBUS_TYPE_UINT16 || variant_type == DBUS_TYPE_UINT32 ||
				variant_type == DBUS_TYPE_INT16 || variant_type == DBUS_TYPE_INT32) {
			dbus_message_iter_get_basic(&variant, &int_value);
			if(!strcmp(key, "addr")) {
				addr = int_value & 0xFFFF;
			} else if(!strcmp(key, "state")) {
				state = int_value & 0xFFFF;
			} else if(!strcmp(key, "time")) {
				time = int_value & 0xFFFF;
			} else if(!strcmp(key, "delay")) {
				delay = int_value & 0xFFFF;
			}
		}
		dbus_message_iter_next(&dict);
	}

	bt_shell_printf("[dbus][%s][%s] %s.%s"
		"(addr = %4.4x, state = %d, time = %d, delay = %d)\n",
		dbus_message_get_destination(msg),
		dbus_message_get_path(msg),
		dbus_message_get_interface(msg),
		dbus_message_get_member(msg),
		addr, state, time ,delay);

	if (array_len > 2) {
		error = bt_shell_manual_input_fmt("level.move %d %d %d %d",
						addr, state, time, delay);
	} else {
		return meshd_error_invalid_args(msg);
	}

	reply = dbus_message_new_method_return(msg);

	return reply;
}

static const GDBusMethodTable level_methods[] = {
	{
		GDBUS_METHOD("get",
		GDBUS_ARGS({ "model_info", "a{sv}" }),
		NULL,
		exec_level_get)
	},
	{
		GDBUS_METHOD("set",
		GDBUS_ARGS({ "model_info", "a{sv}" }),
		NULL,
		exec_level_set)
	},
	{
		GDBUS_METHOD("delta",
		GDBUS_ARGS({ "model_info", "a{sv}" }),
		NULL,
		exec_delta_set)
	},
	{
		GDBUS_METHOD("move",
		GDBUS_ARGS({ "model_info", "a{sv}" }),
		NULL,
		exec_move_set)
	},
	{ }
};

int meshd_level_register()
{
	gboolean status;
	DBusConnection *dbus_conn;

	dbus_conn = meshd_get_dbus_connection();

	status = g_dbus_register_interface(dbus_conn,
		MESHCTLD_OBJECT_PATH_MODEL_LEVEL, MESHCTLD_DBUS_MESH_MODEL_INTERFACE,
		level_methods, NULL,
		NULL, NULL, NULL);

	if(!status) return -EINVAL;

	return 0;
}
