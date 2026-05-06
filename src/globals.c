#include "ride_dispatch.h"

const char *status_label[] = {
    "INACTIVE", "AVAILABLE", "BUSY", "OFFLINE"
};

Driver driver_registry[MAX_DRIVERS + 1];
QuadNode *g_root = NULL;
int g_total_inserted = 0;
int g_next_id = 1;
