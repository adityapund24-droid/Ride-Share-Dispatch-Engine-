#include "ride_dispatch.h"

int bbox_contains_point(const BBox *b, double x, double y) {
    return (x >= b->x_min && x <= b->x_max && y >= b->y_min && y <= b->y_max);
}

int bbox_intersects_circle(const BBox *b, double cx, double cy, double r) {
    double near_x = fmax(b->x_min, fmin(cx, b->x_max));
    double near_y = fmax(b->y_min, fmin(cy, b->y_max));
    double dx = cx - near_x;
    double dy = cy - near_y;
    return (dx*dx + dy*dy) <= (r*r);
}

double bbox_min_dist_sq(const BBox *b, double cx, double cy) {
    double near_x = fmax(b->x_min, fmin(cx, b->x_max));
    double near_y = fmax(b->y_min, fmin(cy, b->y_max));
    double dx = cx - near_x;
    double dy = cy - near_y;
    return dx*dx + dy*dy;
}

int get_quadrant(const BBox *b, double x, double y) {
    double mid_x = (b->x_min + b->x_max) * 0.5;
    double mid_y = (b->y_min + b->y_max) * 0.5;
    if (x < mid_x) return (y >= mid_y) ? NW : SW;
    return (y >= mid_y) ? NE : SE;
}

BBox get_child_bbox(const BBox *p, int quadrant) {
    double mid_x = (p->x_min + p->x_max) * 0.5;
    double mid_y = (p->y_min + p->y_max) * 0.5;
    switch (quadrant) {
        case NW: return (BBox){p->x_min, mid_y,    mid_x,    p->y_max};
        case NE: return (BBox){mid_x,    mid_y,    p->x_max, p->y_max};
        case SW: return (BBox){p->x_min, p->y_min, mid_x,    mid_y  };
        case SE: return (BBox){mid_x,    p->y_min, p->x_max, mid_y  };
    }
    return (BBox){0,0,0,0};
}

QuadNode *create_node(BBox bbox) {
    QuadNode *node = (QuadNode *)malloc(sizeof(QuadNode));
    if (!node) { fprintf(stderr, "OOM: create_node\n"); exit(1); }
    node->bbox = bbox;
    node->driver = NULL;
    node->is_leaf = 1;
    for (int i = 0; i < 4; i++) node->child[i] = NULL;
    return node;
}

static void subdivide(QuadNode *node) {
    for (int i = 0; i < 4; i++) {
        node->child[i] = create_node(get_child_bbox(&node->bbox, i));
    }
    node->is_leaf = 0;
    if (node->driver) {
        int q = get_quadrant(&node->bbox, node->driver->x, node->driver->y);
        node->child[q]->driver = node->driver;
        node->driver = NULL;
    }
}

void qt_insert(QuadNode *node, Driver *driver) {
    if (!bbox_contains_point(&node->bbox, driver->x, driver->y)) return;

    if (node->is_leaf) {
        if (node->driver == NULL) {
            node->driver = driver;
        } else if (node->driver->id == driver->id) {
            node->driver = driver;
        } else {
            double w = node->bbox.x_max - node->bbox.x_min;
            double h = node->bbox.y_max - node->bbox.y_min;
            if (w < MIN_CELL_SIZE || h < MIN_CELL_SIZE) {
                node->driver = driver;
                return;
            }
            subdivide(node);
            qt_insert(node->child[get_quadrant(&node->bbox, driver->x, driver->y)], driver);
        }
    } else {
        qt_insert(node->child[get_quadrant(&node->bbox, driver->x, driver->y)], driver);
    }
}

int qt_delete(QuadNode *node, int driver_id, double x, double y) {
    if (!bbox_contains_point(&node->bbox, x, y)) return 0;
    if (node->is_leaf) {
        if (node->driver && node->driver->id == driver_id) {
            node->driver = NULL;
            return 1;
        }
        return 0;
    }
    return qt_delete(node->child[get_quadrant(&node->bbox, x, y)], driver_id, x, y);
}

void update_driver(QuadNode *root, int driver_id, double new_x, double new_y) {
    if (driver_id < 1 || driver_id > MAX_DRIVERS) {
        printf("  [!] Invalid driver_id %d\n", driver_id);
        return;
    }

    Driver *d = &driver_registry[driver_id];
    if (IS_ACTIVE(*d)) qt_delete(root, driver_id, d->x, d->y);

    d->id = driver_id;
    d->x = new_x;
    d->y = new_y;

    if (!IS_ACTIVE(*d)) {
        d->status = STATUS_AVAILABLE;
        d->trips_done = 0;
        d->current_rider = -1;
    }
    qt_insert(root, d);
}

void free_tree(QuadNode *node) {
    if (!node) return;
    for (int i = 0; i < 4; i++) free_tree(node->child[i]);
    free(node);
}
