#include "ride_dispatch.h"

int set_driver_status(int driver_id, DriverStatus new_status) {
    if (driver_id < 1 || driver_id > MAX_DRIVERS) return 0;
    Driver *d = &driver_registry[driver_id];
    if (!IS_ACTIVE(*d)) {
        printf("  [!] Driver %d not in system.\n", driver_id);
        return 0;
    }

    DriverStatus old = d->status;
    if (new_status == STATUS_BUSY && old != STATUS_AVAILABLE) {
        printf("  [!] Driver %d is %s - cannot mark BUSY.\n", driver_id, status_label[old]);
        return 0;
    }
    if (new_status == STATUS_AVAILABLE && old == STATUS_INACTIVE) {
        printf("  [!] Driver %d is INACTIVE.\n", driver_id);
        return 0;
    }

    d->status = new_status;
    if (new_status == STATUS_AVAILABLE) d->current_rider = -1;

    printf("  [OK] Driver %d: %s -> %s\n", driver_id, status_label[old], status_label[new_status]);
    return 1;
}

Driver *dispatch_nearest(QuadNode *root, double cx, double cy, int rider_id) {
    NNResult nn = { .driver = NULL, .dist_sq = DBL_MAX };
    qt_nearest_neighbor(root, cx, cy, 1, &nn);
    if (!nn.driver) {
        printf("  [!] No available drivers found.\n");
        return NULL;
    }
    nn.driver->status = STATUS_BUSY;
    nn.driver->current_rider = rider_id;
    printf("  [OK] Driver #%d dispatched to rider #%d  (dist=%.4f)\n",
           nn.driver->id, rider_id, sqrt(nn.dist_sq));
    return nn.driver;
}

int complete_trip(int driver_id) {
    if (driver_id < 1 || driver_id > MAX_DRIVERS) return 0;
    Driver *d = &driver_registry[driver_id];
    if (d->status != STATUS_BUSY) {
        printf("  [!] Driver %d is not currently BUSY.\n", driver_id);
        return 0;
    }
    d->trips_done++;
    d->status = STATUS_AVAILABLE;
    d->current_rider = -1;
    printf("  [OK] Driver #%d trip complete. Total trips: %d\n", driver_id, d->trips_done);
    return 1;
}
