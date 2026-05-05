#include "ride_dispatch.h"

void qt_range_query(QuadNode *node, double cx, double cy, double radius,
                    int available_only, RangeResult *res) {
    if (!node || res->count >= MAX_RESULTS) return;
    if (!bbox_intersects_circle(&node->bbox, cx, cy, radius)) return;

    if (node->is_leaf) {
        if (node->driver) {
            if (available_only && !IS_AVAILABLE(*node->driver)) return;
            double dx = cx - node->driver->x;
            double dy = cy - node->driver->y;
            if ((dx*dx + dy*dy) <= (radius*radius)) res->list[res->count++] = node->driver;
        }
        return;
    }

    for (int i = 0; i < 4; i++)
        if (node->child[i]) qt_range_query(node->child[i], cx, cy, radius, available_only, res);
}

void qt_nearest_neighbor(QuadNode *node, double cx, double cy,
                         int available_only, NNResult *best) {
    if (!node) return;
    if (bbox_min_dist_sq(&node->bbox, cx, cy) >= best->dist_sq) return;

    if (node->is_leaf) {
        if (node->driver) {
            if (available_only && !IS_AVAILABLE(*node->driver)) return;
            double dx = cx - node->driver->x;
            double dy = cy - node->driver->y;
            double d2 = dx*dx + dy*dy;
            if (d2 < best->dist_sq) {
                best->dist_sq = d2;
                best->driver = node->driver;
            }
        }
        return;
    }

    int first = get_quadrant(&node->bbox, cx, cy);
    if (node->child[first]) qt_nearest_neighbor(node->child[first], cx, cy, available_only, best);
    for (int i = 0; i < 4; i++)
        if (i != first && node->child[i]) qt_nearest_neighbor(node->child[i], cx, cy, available_only, best);
}
