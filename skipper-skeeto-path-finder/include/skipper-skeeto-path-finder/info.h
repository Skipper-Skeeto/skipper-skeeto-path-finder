#pragma once

#define DATA_TYPE_RAW 0
#define DATA_TYPE_GRAPH 1
#define USED_DATA_TYPE DATA_TYPE_GRAPH

#define ROOM_COUNT 45
#define ITEM_COUNT 40
#define TASK_COUNT 33
#define STATE_ROOM_INDEX_SIZE 6
#define STATE_TASK_ITEM_START STATE_ROOM_INDEX_SIZE
#define STATE_TASK_ITEM_SIZE 58

#define THREAD_COUNT 8

#define POOL_TOTAL_BYTES 4000000000
#define POOL_COUNT (POOL_TOTAL_BYTES / ((THREAD_COUNT + 1) * sizeof(GraphPath))) // Note that we add temp pool for splitting
#define VERTICES_COUNT 46
#define EDGES_COUNT 176
