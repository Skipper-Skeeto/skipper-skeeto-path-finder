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

#define VERTICES_COUNT 46
#define CURRENT_VERTEX_BITS 6
#define DISTANCE_BITS 7
#define FOCUSED_NEXT_PATH_BITS 5
#define EDGES_COUNT 176
#define THREAD_DISTRIBUTE_LEVEL 4   // At what level we start distributing to threads
#define DISTRIBUTION_LEVEL_LIMIT 15 // At what level we stop distributing but always finish first non-exhausted thread first