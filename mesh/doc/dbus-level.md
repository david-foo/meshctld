# DBus Interface - Level

## DBus Object

| Dest       | Interface             | Object Path            |
| ---------- | --------------------- | ---------------------- |
| org.embest | org.embest.mesh.Model | org/embest/model/level |

## DBus Method

| Member | Arguments In | Arguments Out | Description              |
| ------ | ------------ | ------------- | ------------------------ |
| get    | a{sv}        | null          | Get an level state       |
| set    | a{sv}        | null          | Set an level state       |
| delta  | a{sv}        | null          | Set an level delta state |
| move   | a{sv}        | null          | Set an level move state  |

### get

Get an level model state, A signal with the new state will be emitted then.

| Argument In | Type  | Description |
| ----------- | ----- | ----------- |
|             | a{sv} |             |

| Dict Key | Dict Variant Type | Description            |
| -------- | ----------------- | ---------------------- |
| "addr"   | q                 | Should be a group addr |

### set

Set an level model state. "time" and "delay" can be optional,if "time" is present then delay must also be present.

| Argument In | Type  | Description |
| ----------- | ----- | ----------- |
| new state   | a{sv} |             |

| Dict Key | Dict Variant Type | Description            |
| -------- | ----------------- | ---------------------- |
| "addr"   | q                 | Should be a group addr |
| "state"  | n                 | -32768 ~ 32767         |
| "time"   | q                 | 0ms ~ 10.5 hours (ms)  |
| "delay"  | q                 | 0 ~ 1275 ms (ms)       |

### delta

Set an delta to the current level model state. "time" and "delay" can be optional,if "time" is present then delay must also be present.

| Argument In | Type  | Description |
| ----------- | ----- | ----------- |
| new state   | a{sv} |             |

| Dict Key | Dict Variant Type | Description            |
| -------- | ----------------- | ---------------------- |
| "addr"   | q                 | Should be a group addr |
| "state"  | n                 | -32768 ~ 32767         |
| "time"   | q                 | 0ms ~ 10.5 hours (ms)  |
| "delay"  | q                 | 0 ~ 1275 ms (ms)       |

### move

Set an delta to the current level model state. "time" and "delay" are both mandatory.

| Argument In | Type  | Description |
| ----------- | ----- | ----------- |
| new state   | a{sv} |             |

| Dict Key | Dict Variant Type | Description            |
| -------- | ----------------- | ---------------------- |
| "addr"   | q                 | Should be a group addr |
| "state"  | n                 | -32768 ~ 32767         |
| "time"   | q                 | 0ms ~ 10.5 hours (ms)  |
| "delay"  | q                 | 0 ~ 1275 ms (ms)       |

## DBus Signals

| Member | Arguments | Description              |
| ------ | --------- | ------------------------ |
| status | a{sv}     | The new state of a model |

### status

| Dict Key         | Dict Variant Type | Description                                                             |
| ---------------- | ----------------- | ----------------------------------------------------------------------- |
| "object"         | s                 | Object Path, will be 'org/embest/model/level'                           |
| "mod_id"         | q                 | Model Id, it will be '1002', Generic level Server                       |
| "src"            | q                 | The state from which element model, it is the unicast addr of the model |
| "dst"            | q                 | The msg send to                                                         |
| "present_level"  | n                 | target level state, -32768 ~ 32767                                      |
| "target_level"   | n                 | target level state, -32768 ~ 32767                                      |
| "remaining_time" | q                 | remaining time (ms)                                                     |
