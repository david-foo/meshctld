/*
 *
 *  embest generic level model support
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdbool.h>
#include <sys/uio.h>
#include <wordexp.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <glib.h>

#include "src/shared/shell.h"
#include "src/shared/util.h"
#include "mesh/mesh-net.h"
#include "mesh/keys.h"
#include "mesh/net.h"
#include "mesh/node.h"
#include "mesh/prov-db.h"
#include "mesh/util.h"
#include "mesh/level-model.h"

#include "mesh/dbus-server.h"
#include "mesh/meshd-shell.h"
#include "mesh/meshd-model.h"
#include "mesh/meshd-level-model.h"
#include "mesh/cli-model.h"
#include "mesh/model.h"

static uint8_t trans_id;
static uint16_t level_app_idx = APP_IDX_INVALID;

static bool client_level_status_msg_recvd(uint16_t src, uint16_t dst,
						uint8_t *data, uint16_t len, void *user_data);

static int client_bind(uint16_t app_idx, int action)
{
	if (action == ACTION_ADD) {
		if (level_app_idx != APP_IDX_INVALID) {
			return MESH_STATUS_INSUFF_RESOURCES;
		} else {
			level_app_idx = app_idx;
			bt_shell_printf("On/Off client model: new binding"
					" %4.4x\n", app_idx);
		}
	} else {
		if (level_app_idx == app_idx)
			level_app_idx = APP_IDX_INVALID;
	}
	return MESH_STATUS_SUCCESS;
}

static int tt2time(uint8_t remaining_time)
{
	uint8_t step = (remaining_time & 0xc0) >> 6;
	uint8_t count = remaining_time & 0x3f;
	int secs = 0, msecs = 0, minutes = 0, hours = 0;

	switch (step) {
	case 0:
		msecs = 100 * count;
		secs = msecs / 1000;
		msecs -= (secs * 1000);
		break;
	case 1:
		secs = 1 * count;
		minutes = secs / 60;
		secs -= (minutes * 60);
		break;

	case 2:
		secs = 10 * count;
		minutes = secs / 60;
		secs -= (minutes * 60);
		break;
	case 3:
		minutes = 10 * count;
		hours = minutes / 60;
		minutes -= (hours * 60);
		break;

	default:
		break;
	}

	//bt_shell_printf("\n\t\tRemaining time: %d hrs %d mins %d secs %d"
	//		" msecs\n", hours, minutes, secs, msecs);

	return hours * 3600000 + minutes * 60000 + secs * 1000 + msecs;
}

static uint8_t time2tt(uint32_t timems)
{
	uint8_t steps = 0;
	uint8_t resolution = 0;
	if (timems > 620000)
	{
		// > 620 seconds -> resolution=0b11 [10 minutes]
		resolution = 0x03;
		steps = timems / 600000;
	}
	else if (timems > 62000)
	{
		// > 62 seconds -> resolution=0b10 [10 seconds]
		resolution = 0x02;
		steps = timems / 10000;
	}
	else if (timems > 6200)
	{
		// > 6.2 seconds -> resolution=0b01 [1 seconds]
		resolution = 0x01;
		steps = timems / 1000;
	}
	else
	{
		// <= 6.2 seconds -> resolution=0b00 [100 ms]
		resolution = 0x00;
		steps = timems / 100;
	}
	//bt_shell_printf("calculated steps=%d,resolution=%d\n", steps, resolution);
	return ((resolution << 6) | steps);
}

static bool client_msg_recvd(uint16_t src, uint8_t *data,
				uint16_t len, void *user_data)
{
	uint32_t opcode;
	int n;
	uint16_t local_node_unicast;

	local_node_unicast = node_get_primary(node_get_local_node());

	if (mesh_opcode_get(data, len, &opcode, &n)) {
		len -= n;
		data += n;
	} else
		return false;

	bt_shell_printf("Level Model Message received (%d) opcode %x\n",
								len, opcode);
	print_byte_array("\t",data, len);

	switch (opcode & ~OP_UNRELIABLE) {
	default:
		return false;

	case OP_GENERIC_LEVEL_STATUS:
		return client_level_status_msg_recvd(src, local_node_unicast,
				data, len, NULL);
		break;
	}

	return true;
}


static uint32_t parms[8];

static uint32_t read_input_parameters(int argc, char *argv[])
{
	uint32_t i;

	if (!argc)
		return 0;

	--argc;
	++argv;

	if (!argc || argv[0][0] == '\0')
		return 0;

	memset(parms, 0xff, sizeof(parms));

	for (i = 0; i < sizeof(parms)/sizeof(parms[0]) && i < (unsigned) argc;
									i++) {
		sscanf(argv[i], "%d", &parms[i]);
		if (parms[i] == 0xffffffff)
			break;
	}

	return i;
}

static void cmd_level_get(int argc, char *argv[])
{
	uint16_t target;
	uint16_t n;
	uint8_t msg[32];
	struct mesh_node *node;

	if ((read_input_parameters(argc, argv) != 1)) {
		bt_shell_printf("Bad arguments\n");
		level_emit_cmd_failed(argv[0], -EINVAL, "Bad arguments");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	target = parms[0];

	if (IS_UNASSIGNED(target)) {
		bt_shell_printf("Invalid target address\n");
		level_emit_cmd_failed(argv[0], -EINVAL, "Invalid target address");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	if (IS_UNICAST(target)) {
		node = node_find_by_addr(target);

		if (!node) {
			bt_shell_printf("Invalid target address\n");
			level_emit_cmd_failed(argv[0], -EINVAL, "Invalid target address");
			return bt_shell_noninteractive_quit(EXIT_FAILURE);
		}
	}

	n = mesh_opcode_set(OP_GENERIC_LEVEL_GET, msg);

	if (!send_cmd(target, msg, n)) {
		bt_shell_printf("Failed to send \"GENERIC LEVEL GET\"\n");
		level_emit_cmd_failed(argv[0], -EFAULT, "Failed to send GENERIC LEVEL GET");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_level_set(int argc, char *argv[])
{
	uint8_t para_n;
	uint16_t target;
	uint16_t n;
	uint8_t msg[32];
	struct mesh_node *node;

	para_n = read_input_parameters(argc, argv);
	if ((para_n != 2)  &&  (para_n != 4)) {
		bt_shell_printf("Bad arguments: Expecting addr state OR addr state time delay");
		level_emit_cmd_failed(argv[0], -EINVAL, "Bad arguments");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	target = parms[0];

	if (IS_UNASSIGNED(target)) {
		bt_shell_printf("Invalid target address\n");
		level_emit_cmd_failed(argv[0], -EINVAL, "Invalid target address");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	if (IS_UNICAST(target)) {
		node = node_find_by_addr(target);

		if (!node) {
			bt_shell_printf("Invalid target address\n");
			level_emit_cmd_failed(argv[0], -EINVAL, "Invalid target address");
			return bt_shell_noninteractive_quit(EXIT_FAILURE);
		}
	}

	n = mesh_opcode_set(OP_GENERIC_LEVEL_SET, msg);
	msg[n++] = parms[1] % 256;
	msg[n++] = parms[1] / 256;
	msg[n++] = trans_id++;
	if (para_n == 4) {
		msg[n++] = time2tt(parms[2]);
		msg[n++] = parms[3] / 5;
	}

	if (!send_cmd(target, msg, n)) {
		bt_shell_printf("Failed to send \"GENERIC LEVEL SET\"\n");
		level_emit_cmd_failed(argv[0], -EFAULT, "Failed to send GENERIC LEVEL SET");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_delta_set(int argc, char *argv[])
{
	uint8_t para_n;
	uint16_t target;
	uint16_t n;
	uint8_t msg[32];
	struct mesh_node *node;

	para_n = read_input_parameters(argc, argv);
	if ((para_n != 2)  &&  (para_n != 4)) {
		bt_shell_printf("Bad arguments: Expecting addr state OR addr state time delay");
		level_emit_cmd_failed(argv[0], -EINVAL, "Bad arguments");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	target = parms[0];

	if (IS_UNASSIGNED(target)) {
		bt_shell_printf("Invalid target address\n");
		level_emit_cmd_failed(argv[0], -EINVAL, "Invalid target address");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	if (IS_UNICAST(target)) {
		node = node_find_by_addr(target);

		if (!node) {
			bt_shell_printf("Invalid target address\n");
			level_emit_cmd_failed(argv[0], -EINVAL, "Invalid target address");
			return bt_shell_noninteractive_quit(EXIT_FAILURE);
		}
	}

	n = mesh_opcode_set(OP_GENERIC_DELTA_SET, msg);
	msg[n++] = parms[1] % 256;
	msg[n++] = parms[1] / 256;
	msg[n++] = 0;
	msg[n++] = 0;
	msg[n++] = trans_id++;
	if (para_n == 4) {
		msg[n++] = time2tt(parms[2]);
		msg[n++] = parms[3] / 5;
	}

	if (!send_cmd(target, msg, n)) {
		bt_shell_printf("Failed to send \"GENERIC DELTA SET\"\n");
		level_emit_cmd_failed(argv[0], -EFAULT, "Failed to send GENERIC DELTA SET");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static void cmd_move_set(int argc, char *argv[])
{
	uint8_t para_n;
	uint16_t target;
	uint16_t n;
	uint8_t msg[32];
	struct mesh_node *node;

	para_n = read_input_parameters(argc, argv);
	if ((para_n != 2)  &&  (para_n != 4)) {
		bt_shell_printf("Bad arguments: Expecting addr state OR addr state time delay");
		level_emit_cmd_failed(argv[0], -EINVAL, "Bad arguments");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	target = parms[0];

	if (IS_UNASSIGNED(target)) {
		bt_shell_printf("Invalid target address\n");
		level_emit_cmd_failed(argv[0], -EINVAL, "Invalid target address");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	if (IS_UNICAST(target)) {
		node = node_find_by_addr(target);

		if (!node) {
			bt_shell_printf("Invalid target address\n");
			level_emit_cmd_failed(argv[0], -EINVAL, "Invalid target address");
			return bt_shell_noninteractive_quit(EXIT_FAILURE);
		}
	}

	n = mesh_opcode_set(OP_GENERIC_MOVE_SET, msg);
	msg[n++] = parms[1] % 256;
	msg[n++] = parms[1] / 256;
	msg[n++] = trans_id++;
	if (para_n == 4) {
		msg[n++] = time2tt(parms[2]);
		msg[n++] = parms[3] / 5;
	}

	if (!send_cmd(target, msg, n)) {
		bt_shell_printf("Failed to send \"GENERIC MOVE SET\"\n");
		level_emit_cmd_failed(argv[0], -EFAULT, "Failed to send GENERIC MOVE SET");
		return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

static const struct bt_shell_menu level_menu = {
	.name = "level",
	.desc = "level Model Submenu",
	.entries = {
	{"get",		"<ele_addr>",								cmd_level_get,			"Get LEVEL status"},
	{"set",		"<ele_addr> <level> [time] [delay]",		cmd_level_set,			"Set LEVEL status"},
	{"delta",		"<ele_addr> <delta> [time] [delay]",		cmd_delta_set,			"Set DELTA status"},
	{"move",		"<ele_addr> <move> <time> <delay>",		cmd_move_set,			"Set MOVE status"},
	{}
	},
};

//static struct mesh_model_ops client_cbs = {
//	client_msg_recvd,
//	client_bind,
//	NULL,
//	NULL
//};

static bool client_level_status_msg_recvd(uint16_t src, uint16_t dst,
						uint8_t *data, uint16_t len, void *user_data)
{
	uint16_t present_level, target_level;
	int remaining_ms = -1;

	if (len != 2 && len != 5) {
		return false;
	}

	present_level = data[1]<<8 | data[0];
	bt_shell_printf("Element %4.4x: Level Status present = %x",
					src, present_level);

	if (len == 5) {
		target_level = data[3]<<8 | data[2];
		remaining_ms = tt2time(data[4]);
		bt_shell_printf(", target_level = %x, remaining_ms=%d", target_level, remaining_ms);
		level_emit_new_state_with_remaining(src, dst, present_level, target_level, remaining_ms);
	} else {
		bt_shell_printf("\n");
		level_emit_new_state(src, dst, present_level);
	}
	return true;
}

static struct mesh_opcode_op level_ops[] = {
	{"level status", OP_GENERIC_LEVEL_STATUS, client_level_status_msg_recvd, NULL},
	MESH_OPCODE_OP_END
};

bool level_client_init(void)
{
	if(!node_remote_opcode_register(level_ops))
		return false;

	bt_shell_add_submenu(&level_menu);

	return true;
}
