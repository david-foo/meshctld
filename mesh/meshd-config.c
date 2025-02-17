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
#include "src/shared/queue.h"
#include "src/shared/shell.h"
#include "mesh/dbus-server.h"
#include "mesh/meshd-config.h"

static DBusMessage *exec_config_target(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *reply;
	DBusMessageIter iter, array;
	dbus_uint16_t unicast;
	int error;

	if (dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT16, &unicast,
			DBUS_TYPE_INVALID) == FALSE)
		return meshd_error_invalid_args(msg);

	bt_shell_printf("[dbus][%s][%s] %s.%s(%4.4x)\n",
		dbus_message_get_destination(msg),
		dbus_message_get_path(msg),
		dbus_message_get_interface(msg),
		dbus_message_get_member(msg),
		unicast);

	error = bt_shell_manual_input_fmt("config.target %4.4x", unicast);

	reply = dbus_message_new_method_return(msg);

	return reply;
}

static DBusMessage *exec_config_appkey_add(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *reply;
	DBusMessageIter iter, array;
	dbus_uint16_t app_key_idx;
	int error;

	if (dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT16, &app_key_idx,
			DBUS_TYPE_INVALID) == FALSE)
		return meshd_error_invalid_args(msg);

	bt_shell_printf("[dbus][%s][%s] %s.%s(%4.4x)\n",
		dbus_message_get_destination(msg),
		dbus_message_get_path(msg),
		dbus_message_get_interface(msg),
		dbus_message_get_member(msg),
		app_key_idx);

	error = bt_shell_manual_input_fmt("config.appkey_add %4.4x", app_key_idx);

	reply = dbus_message_new_method_return(msg);

	return reply;
}

static DBusMessage *exec_config_appkey_del(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *reply;
	DBusMessageIter iter, array;
	dbus_uint16_t app_key_idx;
	int error;

	if (dbus_message_get_args(msg, NULL, DBUS_TYPE_UINT16, &app_key_idx,
			DBUS_TYPE_INVALID) == FALSE)
		return meshd_error_invalid_args(msg);

	bt_shell_printf("[dbus][%s][%s] %s.%s(%4.4x)\n",
		dbus_message_get_destination(msg),
		dbus_message_get_path(msg),
		dbus_message_get_interface(msg),
		dbus_message_get_member(msg),
		app_key_idx);

	error = bt_shell_manual_input_fmt("config.appkey_del %4.4x", app_key_idx);

	reply = dbus_message_new_method_return(msg);

	return reply;
}

// sudo gdbus call -y -d org.embest -o /org/embest/Config -m org.embest.MeshInterface.bind "{'ele_idx': <0>, 'app_idx': <1>, 'mod_id': <0x1000>}"
static DBusMessage *exec_config_bind(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *reply;
	DBusMessageIter array, dict, entry, variant;
	int error;
	int i, array_len, int_value;
	int variant_type;
	const char* key;
	uint16_t ele_idx, app_idx, mod_id;

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
			if(!strcmp(key, "ele_idx")) {
				ele_idx = int_value;
			} else if(!strcmp(key, "app_idx")) {
				app_idx = int_value;
			} else if(!strcmp(key, "mod_id")) {
				mod_id = int_value;
			}
		}
		dbus_message_iter_next(&dict);
	}

	bt_shell_printf("[dbus][%s][%s] %s.%s"
		"(ele_idx = %d, app_idx = %d, mod_id = %4.4x)\n",
		dbus_message_get_destination(msg),
		dbus_message_get_path(msg),
		dbus_message_get_interface(msg),
		dbus_message_get_member(msg),
		ele_idx, app_idx, mod_id);

	error = bt_shell_manual_input_fmt("config.bind %d %d %4.4x", ele_idx, app_idx, mod_id);

	reply = dbus_message_new_method_return(msg);

	return reply;
}

/*
sudo gdbus call -y -d org.embest -o /org/embest/Config -m org.embest.MeshInterface.pub_set \
 "{'ele_addr': <0x0107>, 'pub_addr': <0xceef>, 'app_idx': <1>, 'mod_id': <0x1001>, 'period': <0>}"
*/
static DBusMessage *exec_config_pub_set(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *reply;
	DBusMessageIter array, dict, entry, variant;
	int error;
	int i, array_len, int_value;
	int variant_type;
	const char* key;
	uint16_t ele_addr, pub_addr = 0, app_idx, mod_id;
	int period = 0, period_step = 0, period_res = 0;
	int retransmit_count = 0, retransmit_steps = 0;

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
			if(!strcmp(key, "ele_addr")) {
				ele_addr = int_value & 0xFFFF;
			} else if(!strcmp(key, "pub_addr")) {
				pub_addr = int_value & 0xFFFF;
			} else if(!strcmp(key, "app_idx")) {
				app_idx = int_value;
			} else if(!strcmp(key, "mod_id")) {
				mod_id = int_value;
			} else if(!strcmp(key, "period")) {
				period = int_value;
			} else if(!strcmp(key, "retransmit_count")) {
				retransmit_count = int_value;
			} else if(!strcmp(key, "retransmit_steps")) {
				retransmit_steps = int_value;
			}
		}
		dbus_message_iter_next(&dict);
	}

	bt_shell_printf("[dbus][%s][%s] %s.%s"
		"(ele_addr = %4.4x, pub_addr = %4.4x, app_idx = %d, mod_id = %4.4x, "
		"period = %dms, "
		"retransmit_count = %d, retransmit_steps = %d"
		")\n",
		dbus_message_get_destination(msg),
		dbus_message_get_path(msg),
		dbus_message_get_interface(msg),
		dbus_message_get_member(msg),
		ele_addr, pub_addr, app_idx, mod_id,
		period,
		retransmit_count, retransmit_steps);

	if(period >= 1000 * 60 * 10) {
		period_res = period / (1000 * 60 * 10);
		period_step = 3;
	} else if(period >= 1000) {
		period_res = period / 1000;
		period_step = 1;
	} else {
		period_res = period / 100;
		period_step = 0;
	}

	error = bt_shell_manual_input_fmt("config.pub_set %4.4x %4.4x %d %x %d %4.4x",
						ele_addr, pub_addr, app_idx,
						((period_step & 0x03) << 6) | (period_res & 0x3F),
						((retransmit_count & 0x07) << 5) | (retransmit_steps & 0x1F),
						mod_id);

	reply = dbus_message_new_method_return(msg);

	return reply;
}

/*
sudo gdbus call -y -d org.embest -o /org/embest/Config -m org.embest.MeshInterface.sub_add \
 "{'ele_addr': <0x0107>, 'sub_addr': <0xceef>, 'mod_id': <0x1001>}"
*/
static DBusMessage *exec_config_sub_add(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *reply;
	DBusMessageIter array, dict, entry, variant;
	int error;
	int i, array_len, int_value;
	int variant_type;
	const char* key;
	uint16_t ele_addr, sub_addr = 0, mod_id;

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
			if(!strcmp(key, "ele_addr")) {
				ele_addr = int_value & 0xFFFF;
			} else if(!strcmp(key, "sub_addr")) {
				sub_addr = int_value & 0xFFFF;
			} else if(!strcmp(key, "mod_id")) {
				mod_id = int_value;
			}
		}
		dbus_message_iter_next(&dict);
	}

	bt_shell_printf("[dbus][%s][%s] %s.%s"
		"(ele_addr = %4.4x, sub_addr = %4.4x, mod_id = %4.4x)\n",
		dbus_message_get_destination(msg),
		dbus_message_get_path(msg),
		dbus_message_get_interface(msg),
		dbus_message_get_member(msg),
		ele_addr, sub_addr, mod_id);

	error = bt_shell_manual_input_fmt("config.sub_add %4.4x %4.4x %4.4x",
						ele_addr, sub_addr, mod_id);

	reply = dbus_message_new_method_return(msg);

	return reply;
}

static DBusMessage *exec_config_composition_get(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *reply;
	int error;

	bt_shell_printf("[dbus][%s][%s] %s.%s()\n",
		dbus_message_get_destination(msg),
		dbus_message_get_path(msg),
		dbus_message_get_interface(msg),
		dbus_message_get_member(msg));

	error = bt_shell_manual_input_fmt("config.composition_get");

	reply = dbus_message_new_method_return(msg);

	return reply;
}

static DBusMessage *exec_config_node_reset(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *reply;
	int error;

	bt_shell_printf("[dbus][%s][%s] %s.%s()\n",
		dbus_message_get_destination(msg),
		dbus_message_get_path(msg),
		dbus_message_get_interface(msg),
		dbus_message_get_member(msg));

	error = bt_shell_manual_input_fmt("config.node_reset");

	reply = dbus_message_new_method_return(msg);

	return reply;
}

static DBusMessage *exec_config_hb_pub_set(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
	DBusMessage *reply;
	DBusMessageIter array, dict, entry, variant;
	int error;
	int i, array_len, int_value;
	int variant_type;
	const char* key;
	uint16_t pub_addr = 0;
	uint8_t count = 0xFF;
	uint8_t period = 7;		// default 64s 2^(n-1)
	uint8_t ttl = 7;
	uint16_t features = 0;
	uint16_t net_idx = 0;

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
			if(!strcmp(key, "pub_addr")) {
				pub_addr = int_value & 0xFFFF;
			} else if(!strcmp(key, "net_idx")) {
				net_idx = int_value;
			} else if(!strcmp(key, "period")) {
				period = int_value;
			}
		}
		dbus_message_iter_next(&dict);
	}

	bt_shell_printf("[dbus][%s][%s] %s.%s"
		"(pub_addr = %4.4x, count = %d, period = %4.4x, "
		"ttl = %x, features = %4.4x, net_idx = %x"
		")\n",
		dbus_message_get_destination(msg),
		dbus_message_get_path(msg),
		dbus_message_get_interface(msg),
		dbus_message_get_member(msg),
		pub_addr, count, period,
		ttl, features, net_idx);

// hb_pub_set <pub_addr> <count> <period> <ttl> <features> <net_idx> Set heartbeat publish
	error = bt_shell_manual_input_fmt("config.hb_pub_set %4.4x %2.2x %2.2x %x %4.4x %4.4x",
						pub_addr, count, period,
						ttl, features, net_idx);

	reply = dbus_message_new_method_return(msg);

	return reply;
}

static const GDBusMethodTable config_methods[] = {
	{
		GDBUS_METHOD("target",
		GDBUS_ARGS({ "unicast", "q" }),
		NULL,
		exec_config_target)
	},
	{
		GDBUS_METHOD("appkey_add",
		GDBUS_ARGS({ "app_key_idx", "q" }),
		NULL,
		exec_config_appkey_add)
	},
	{
		GDBUS_METHOD("appkey_del",
		GDBUS_ARGS({ "app_key_idx", "q" }),
		NULL,
		exec_config_appkey_del)
	},
	{
		GDBUS_METHOD("bind",
		GDBUS_ARGS({ "info", "a{sv}" }),
		NULL,
		exec_config_bind)
	},
	{
		GDBUS_METHOD("pub_set",
		GDBUS_ARGS({ "info", "a{sv}" }),
		NULL,
		exec_config_pub_set)
	},
	{
		GDBUS_METHOD("sub_add",
		GDBUS_ARGS({ "info", "a{sv}" }),
		NULL,
		exec_config_sub_add)
	},
	{
		GDBUS_METHOD("composition_get",
		NULL,
		NULL,
		exec_config_composition_get)
	},
	{
		GDBUS_METHOD("node_reset",
		NULL,
		NULL,
		exec_config_node_reset)
	},
	{
		GDBUS_METHOD("hb_pub_set",
		GDBUS_ARGS({ "info", "a{sv}" }),
		NULL,
		exec_config_hb_pub_set)
	},
	{ }
};

static const GDBusSignalTable config_signals[] = {
	{ GDBUS_SIGNAL(MESHCTLD_SIGNAL_CONFIG_STATUS, GDBUS_ARGS({ "result", "a{sv}" })) },
	{ }
};


int meshd_config_register()
{
	gboolean status;
	DBusConnection *dbus_conn;

	dbus_conn = meshd_get_dbus_connection();

	status = g_dbus_register_interface(dbus_conn,
		MESHCTLD_OBJECT_PATH_CONFIG, MESHCTLD_DBUS_MESH_INTERFACE,
		config_methods, config_signals,
		NULL, NULL, NULL);

	if(!status) return -EINVAL;

	return 0;
}