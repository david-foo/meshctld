#ifndef _MESHD_LEVEL_MODEL_H
#define _MESHD_LEVEL_MODEL_H

#define level_emit_new_state_with_remaining(src, dst, present_level, target_level, remaining_time) \
				model_emit_status(MESHCTLD_OBJECT_PATH_MODEL_LEVEL, \
					GENERIC_LEVEL_SERVER_MODEL_ID, src, dst, \
					"present_level=%n,target_level=%n,remaining_time=%i", \
					present_level, target_level, remaining_time)

#define level_emit_new_state(src, dst, present_level) \
				model_emit_status(MESHCTLD_OBJECT_PATH_MODEL_LEVEL, \
					GENERIC_LEVEL_SERVER_MODEL_ID, src, dst, \
					"present_level=%n", \
					present_level)

#define level_emit_cmd_failed(method, result, error, ...) \
				shell_emit_cmd_failed(MESHCTLD_OBJECT_PATH_MODEL_LEVEL, \
					method, result, error, ## __VA_ARGS__)


int meshd_level_register();

#endif