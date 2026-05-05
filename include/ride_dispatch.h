#ifndef RIDE_DISPATCH_H
#define RIDE_DISPATCH_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <time.h>

/* Constants & Configuration */
#define MAX_DRIVERS     100000
#define CITY_MIN_X      0.0
#define CITY_MIN_Y      0.0
#define CITY_MAX_X      1000.0
#define CITY_MAX_Y      1000.0
#define MAX_RESULTS     5000
#define MIN_CELL_SIZE   0.0001
#define MAX_KNN         50

#define NW 0
#define NE 1
#define SW 2
#define SE 3

/* Driver status */
typedef enum {
    STATUS_INACTIVE  = 0,
    STATUS_AVAILABLE = 1,
    STATUS_BUSY      = 2,
    STATUS_OFFLINE   = 3
} DriverStatus;

extern const char *status_label[];

typedef struct {
    int          id;
    double       x, y;
    DriverStatus status;
    int          trips_done;
    int          current_rider;
} Driver;

#define IS_ACTIVE(d)    ((d).status != STATUS_INACTIVE)
#define IS_AVAILABLE(d) ((d).status == STATUS_AVAILABLE)

typedef struct {
    double x_min, y_min;
    double x_max, y_max;
} BBox;

typedef struct QuadNode {
    BBox             bbox;
    struct QuadNode *child[4];
    Driver          *driver;
    int              is_leaf;
} QuadNode;

typedef struct {
    Driver *list[MAX_RESULTS];
    int     count;
} RangeResult;

typedef struct {
    Driver *driver;
    double  dist_sq;
} NNResult;

typedef struct {
    double  dist_sq;
    Driver *driver;
} HeapEntry;

typedef struct {
    HeapEntry entries[MAX_KNN];
    int       size;
    int       k;
} KNNHeap;

typedef struct {
    int  total_nodes, leaf_nodes, internal_nodes;
    int  drivers_stored;
    int  available_count, busy_count, offline_count;
    int  max_depth;
    long total_depth;
} TreeStats;

extern Driver driver_registry[MAX_DRIVERS + 1];
extern QuadNode *g_root;
extern int g_total_inserted;
extern int g_next_id;

/* Person 1 - Core Data Structures & Tree Mechanics */
int bbox_contains_point(const BBox *b, double x, double y);
int bbox_intersects_circle(const BBox *b, double cx, double cy, double r);
double bbox_min_dist_sq(const BBox *b, double cx, double cy);
int get_quadrant(const BBox *b, double x, double y);
BBox get_child_bbox(const BBox *p, int quadrant);
QuadNode *create_node(BBox bbox);
void qt_insert(QuadNode *node, Driver *driver);
int qt_delete(QuadNode *node, int driver_id, double x, double y);
void update_driver(QuadNode *root, int driver_id, double new_x, double new_y);
void free_tree(QuadNode *node);

/* Person 2 - Spatial Queries */
void qt_range_query(QuadNode *node, double cx, double cy, double radius,
                    int available_only, RangeResult *res);
void qt_nearest_neighbor(QuadNode *node, double cx, double cy,
                         int available_only, NNResult *best);

/* Person 3 - K-NN */
void heap_swap(KNNHeap *h, int a, int b);
void heap_sift_up(KNNHeap *h, int i);
void heap_sift_down(KNNHeap *h, int i);
void heap_push(KNNHeap *h, Driver *d, double dist_sq);
double heap_worst(const KNNHeap *h);
void heap_sort_ascending(KNNHeap *h);
void qt_knn(QuadNode *node, double cx, double cy, int available_only, KNNHeap *heap);

/* Person 4 - Driver Status & Dispatch Logic */
int set_driver_status(int driver_id, DriverStatus new_status);
Driver *dispatch_nearest(QuadNode *root, double cx, double cy, int rider_id);
int complete_trip(int driver_id);

/* Person 5 - Diagnostics, Brute Force & Performance */
void collect_stats(QuadNode *node, int depth, TreeStats *s);
void print_stats(QuadNode *root);
Driver *brute_force_nn(double cx, double cy, int available_only);
int brute_force_range(double cx, double cy, double r, int available_only);
int brute_force_knn(double cx, double cy, int k, int available_only, HeapEntry *out);

/* Menu helpers */
void flush_stdin(void);
void print_menu(void);
void menu_insert(void);
void menu_range_query(void);
void menu_nearest_neighbor(void);
void menu_knn(void);
void menu_gps_updates(void);
void menu_status_management(void);
void menu_dispatch(void);
void menu_performance(void);

#endif
