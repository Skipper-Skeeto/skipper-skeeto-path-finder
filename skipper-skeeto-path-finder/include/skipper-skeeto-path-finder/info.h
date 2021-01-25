#pragma once

#define DATA_TYPE_RAW 0
#define DATA_TYPE_GRAPH 1

#define USED_DATA_TYPE DATA_TYPE_GRAPH
#define GAME_ID 1

#define THREAD_COUNT 8
#define POOL_TOTAL_BYTES 4000000000
#define POOL_COUNT (POOL_TOTAL_BYTES / ((THREAD_COUNT + 1) * sizeof(GraphPath))) // Note that we add temp pool for splitting

#define TEMP_DIR "temp"
#define TEMP_PATHS_DIR TEMP_DIR "/paths"
#define TEMP_STATES_DIR TEMP_DIR "/states"

#if GAME_ID == 1

#define START_ROOM "Home"
#define ROOM_COUNT 45
#define ITEM_COUNT 40
#define TASK_COUNT 33
#define STATE_ROOM_INDEX_SIZE 6
#define STATE_TASK_ITEM_START STATE_ROOM_INDEX_SIZE
#define STATE_TASK_ITEM_SIZE 57

#if USED_DATA_TYPE == DATA_TYPE_RAW

#define VERTICES_COUNT STATE_TASK_ITEM_SIZE
#define EDGES_COUNT 250

#elif USED_DATA_TYPE == DATA_TYPE_GRAPH

#define VERTICES_COUNT 46
#define EDGES_COUNT 175

#endif

#elif GAME_ID == 2

#define START_ROOM "main_hall"
#define ROOM_COUNT 28
#define ITEM_COUNT 29
#define TASK_COUNT 21
#define STATE_ROOM_INDEX_SIZE 6
#define STATE_TASK_ITEM_START STATE_ROOM_INDEX_SIZE
#define STATE_TASK_ITEM_SIZE 39

#if USED_DATA_TYPE == DATA_TYPE_RAW

#define VERTICES_COUNT STATE_TASK_ITEM_SIZE
#define EDGES_COUNT 154

#elif USED_DATA_TYPE == DATA_TYPE_GRAPH

#error Configuration not supported

#endif

#endif