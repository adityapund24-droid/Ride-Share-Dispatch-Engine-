#include "ride_dispatch.h"

void collect_stats(QuadNode *node, int depth, TreeStats *s) {
    if (!node) return;
    s->total_nodes++;
    if (depth > s->max_depth) s->max_depth = depth;

    if (node->is_leaf) {
        s->leaf_nodes++;
        if (node->driver) {
            s->drivers_stored++;
            s->total_depth += depth;
            switch (node->driver->status) {
                case STATUS_AVAILABLE: s->available_count++; break;
                case STATUS_BUSY:      s->busy_count++; break;
                case STATUS_OFFLINE:   s->offline_count++; break;
                default: break;
            }
        }
    } else {
        s->internal_nodes++;
        for (int i = 0; i < 4; i++) collect_stats(node->child[i], depth + 1, s);
    }
}

void print_stats(QuadNode *root) {
    TreeStats s = {0};
    collect_stats(root, 0, &s);
    printf("+-----------------------------------------+\n");
    printf("|          QUADTREE STATISTICS            |\n");
    printf("+-----------------------------------------+\n");
    printf("|  Total nodes      : %-6d               |\n", s.total_nodes);
    printf("|  Internal nodes   : %-6d               |\n", s.internal_nodes);
    printf("|  Leaf nodes       : %-6d               |\n", s.leaf_nodes);
    printf("|  Drivers indexed  : %-6d               |\n", s.drivers_stored);
    printf("|  -- AVAILABLE     : %-6d               |\n", s.available_count);
    printf("|  -- BUSY          : %-6d               |\n", s.busy_count);
    printf("|  -- OFFLINE       : %-6d               |\n", s.offline_count);
    printf("|  Max tree depth   : %-6d               |\n", s.max_depth);
    if (s.drivers_stored > 0)
        printf("|  Avg driver depth : %-6.1f               |\n", (double)s.total_depth / s.drivers_stored);
    printf("+-----------------------------------------+\n");
}

Driver *brute_force_nn(double cx, double cy, int available_only) {
    Driver *best = NULL;
    double best_d2 = DBL_MAX;
    for (int i = 1; i <= MAX_DRIVERS; i++) {
        if (!IS_ACTIVE(driver_registry[i])) continue;
        if (available_only && !IS_AVAILABLE(driver_registry[i])) continue;
        double dx = cx - driver_registry[i].x;
        double dy = cy - driver_registry[i].y;
        double d2 = dx*dx + dy*dy;
        if (d2 < best_d2){ 
            best_d2 = d2; 
            best = &driver_registry[i]; 
        }
    }
    return best;
}

int brute_force_range(double cx, double cy, double r, int available_only) {
    int count = 0;
    for (int i = 1; i <= MAX_DRIVERS; i++) {
        if (!IS_ACTIVE(driver_registry[i])) continue;
        if (available_only && !IS_AVAILABLE(driver_registry[i])) continue;
        double dx = cx - driver_registry[i].x;
        double dy = cy - driver_registry[i].y;
        if ((dx*dx + dy*dy) <= (r*r)) count++;
    }
    return count;
}

int brute_force_knn(double cx, double cy, int k, int available_only, HeapEntry *out) {
    int n = 0;
    static HeapEntry all[MAX_DRIVERS];
    for (int i = 1; i <= MAX_DRIVERS; i++) {
        if (!IS_ACTIVE(driver_registry[i])) continue;
        if (available_only && !IS_AVAILABLE(driver_registry[i])) continue;
        double dx = cx - driver_registry[i].x;
        double dy = cy - driver_registry[i].y;
        all[n].dist_sq = dx*dx + dy*dy;
        all[n].driver = &driver_registry[i];
        n++;
    }
    for (int i = 1; i < n; i++) {
        HeapEntry key = all[i];
        int j = i - 1;
        while (j >= 0 && all[j].dist_sq > key.dist_sq) {
            all[j + 1] = all[j];
            j--;
        }
        all[j + 1] = key;
    }
    int take = (k < n) ? k : n;
    for (int i = 0; i < take; i++) out[i] = all[i];
    return take;
}
