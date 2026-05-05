#include "ride_dispatch.h"

void flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void print_menu(void) {
    printf("\n");
    printf(" +======================================================+\n");
    printf("|     RIDE-SHARE DISPATCH ENGINE  -  QuadTree          |\n");
    printf(" +======================================================+\n");
    printf("|  1. Insert / Load Drivers                            |\n");
    printf("|  2. Range Query  (all available drivers in radius R) |\n");
    printf("|  3. Nearest-Neighbor  (closest available driver)     |\n");
    printf("|  4. K-Nearest Neighbors  (top-k available drivers)   |\n");
    printf("|  5. Streaming GPS Updates                            |\n");
    printf("|  6. Driver Status Management                         |\n");
    printf("|  7. Dispatch / Complete Trip                         |\n");
    printf("|  8. Performance Summary (QuadTree vs BruteForce)     |\n");
    printf("|  0. Exit                                             |\n");
    printf(" +======================================================+\n");
    printf("  Drivers indexed : %d\n", g_total_inserted);
    printf("  Enter choice : ");
}

void menu_insert(void) {
    printf("\n--------------------------------------------------\n");
    printf("  OPTION 1 : Insert Drivers\n");
    printf("--------------------------------------------------\n");
    printf("  [1] Auto-generate N drivers (random coordinates)\n");
    printf("  [2] Manually enter each driver (ID  X  Y)\n");
    printf("  Choice : ");

    int mode;
    scanf("%d", &mode); flush_stdin();

    if (mode == 1) {
        int n;
        printf("  How many drivers? : ");
        scanf("%d", &n); flush_stdin();
        if (n <= 0) { printf("  [!] Invalid count.\n"); return; }
        if (g_next_id + n - 1 > MAX_DRIVERS) {
            n = MAX_DRIVERS - g_next_id + 1;
            if (n <= 0) { printf("  [!] Registry full.\n"); return; }
        }

        clock_t t0 = clock();
        for (int i = 0; i < n; i++) {
            double x = ((double)rand() / RAND_MAX) * CITY_MAX_X;
            double y = ((double)rand() / RAND_MAX) * CITY_MAX_Y;
            update_driver(g_root, g_next_id, x, y);
            g_next_id++;
        }
        double elapsed = (double)(clock() - t0) / CLOCKS_PER_SEC;
        g_total_inserted += n;
        printf("  Inserted %d drivers in %.4f s  (%.2f us/driver)\n", n, elapsed, (elapsed / n) * 1e6);
        print_stats(g_root);
    } else if (mode == 2) {
        int n;
        printf("  How many drivers? : ");
        scanf("%d", &n); flush_stdin();
        if (n <= 0) { printf("  [!] Invalid count.\n"); return; }

        printf("  City bounds: X=[%.0f,%.0f]  Y=[%.0f,%.0f]\n", CITY_MIN_X, CITY_MAX_X, CITY_MIN_Y, CITY_MAX_Y);
        printf("  Format per line:  DriverID  X  Y\n\n");

        int inserted = 0;
        for (int i = 0; i < n; i++) {
            int id; double x, y;
            printf("  Driver %d/%d -> ", i+1, n);
            if (scanf("%d %lf %lf", &id, &x, &y) != 3) {
                flush_stdin(); printf("  [!] Bad input, skipping.\n"); continue;
            }
            flush_stdin();
            if (id < 1 || id > MAX_DRIVERS) { printf("  [!] ID out of range.\n"); continue; }
            if (x < CITY_MIN_X || x > CITY_MAX_X || y < CITY_MIN_Y || y > CITY_MAX_Y) {
                printf("  [!] Coordinates out of bounds.\n"); continue;
            }
            int was_new = !IS_ACTIVE(driver_registry[id]);
            update_driver(g_root, id, x, y);
            if (was_new) {
                g_total_inserted++;
                if (id >= g_next_id) g_next_id = id + 1;
            }
            printf("  [OK] Driver %d at (%.3f, %.3f) - %s\n", id, x, y, status_label[driver_registry[id].status]);
            inserted++;
        }
        printf("  %d driver(s) inserted.\n", inserted);
        print_stats(g_root);
    } else {
        printf("  [!] Invalid mode.\n");
    }
}

void menu_range_query(void) {
    printf("\n--------------------------------------------------\n  OPTION 2 : Range Query\n--------------------------------------------------\n");

    if (g_total_inserted == 0){
        printf("  [!] No drivers in system. Run Option 1 first.\n");
        return; 
    }

    double cx, cy, r;
    printf("  User X : "); scanf("%lf", &cx); flush_stdin();
    printf("  User Y : "); scanf("%lf", &cy); flush_stdin();
    printf("  Radius : "); scanf("%lf", &r);  flush_stdin();

    if(r <= 0) { 
        printf("  [!] Radius must be positive.\n"); 
        return; 
    }

    int avail;
    printf("  Show all drivers or only AVAILABLE? [0=all / 1=available] : ");
    scanf("%d", &avail); flush_stdin();

    RangeResult res = { .count = 0 };
    clock_t t0 = clock();
    qt_range_query(g_root, cx, cy, r, avail, &res);
    double qt_time = (double)(clock() - t0) / CLOCKS_PER_SEC;

    t0 = clock();
    int bf_count = brute_force_range(cx, cy, r, avail);
    double bf_time = (double)(clock() - t0) / CLOCKS_PER_SEC;

    printf("\n  Drivers found   : %d\n", res.count);
    printf("  Filter          : %s\n", avail ? "AVAILABLE" : "ALL");
    printf("  QuadTree time   : %.6f seconds\n", qt_time);
    printf("  BruteForce time : %.6f seconds\n", bf_time);
    printf("  Correctness     : %s\n", (bf_count == res.count) ? "MATCH" : "MISMATCH");

    if (res.count == 0) return;

    for (int i = 0; i < res.count - 1 && i < 20; i++) {
        for (int j = i + 1; j < res.count; j++) {
            double di = (cx-res.list[i]->x)*(cx-res.list[i]->x) + (cy-res.list[i]->y)*(cy-res.list[i]->y);
            double dj = (cx-res.list[j]->x)*(cx-res.list[j]->x) + (cy-res.list[j]->y)*(cy-res.list[j]->y);
            if (dj < di) { Driver *tmp = res.list[i]; res.list[i] = res.list[j]; res.list[j] = tmp; }
        }
    }

    int show = (res.count > 10) ? 10 : res.count;
    printf("\n  Top %d drivers by distance:\n", show);
    printf("  %-8s  %-9s  %-10s  %-10s  %-10s\n", "ID", "Status", "X", "Y", "Distance");
    for (int i = 0; i < show; i++) {
        double dx = cx - res.list[i]->x;
        double dy = cy - res.list[i]->y;
        printf("  %-8d  %-9s  %-10.4f  %-10.4f  %-10.4f\n",
               res.list[i]->id, status_label[res.list[i]->status], res.list[i]->x, res.list[i]->y, sqrt(dx*dx + dy*dy));
    }
}

void menu_nearest_neighbor(void) {
    printf("\n--------------------------------------------------\n  OPTION 3 : Nearest-Neighbor Query\n--------------------------------------------------\n");
    if (g_total_inserted == 0) { printf("  [!] No drivers in system.\n"); return; }

    double cx, cy;
    printf("  User X : "); scanf("%lf", &cx); flush_stdin();
    printf("  User Y : "); scanf("%lf", &cy); flush_stdin();
    int avail;
    printf("  Only AVAILABLE drivers? [0=no / 1=yes] : "); scanf("%d", &avail); flush_stdin();

    NNResult nn = { .driver = NULL, .dist_sq = DBL_MAX };
    clock_t t0 = clock();
    qt_nearest_neighbor(g_root, cx, cy, avail, &nn);
    double qt_time = (double)(clock() - t0) / CLOCKS_PER_SEC;

    t0 = clock();
    Driver *bf = brute_force_nn(cx, cy, avail);
    double bf_time = (double)(clock() - t0) / CLOCKS_PER_SEC;

    int match = (nn.driver && bf && fabs(nn.dist_sq - ((cx-bf->x)*(cx-bf->x) + (cy-bf->y)*(cy-bf->y))) < 1e-9);

    if (nn.driver) {
        printf("\n  QuadTree Driver : #%d (%.2f, %.2f), time %.6f s\n", nn.driver->id, nn.driver->x, nn.driver->y, qt_time);
        printf("  BruteForce      : #%d (%.2f, %.2f), time %.6f s\n", bf->id, bf->x, bf->y, bf_time);
        printf("  Distance        : %.4f units\n", sqrt(nn.dist_sq));
        printf("  Status          : %s\n", status_label[nn.driver->status]);
        printf("  Correctness     : %s\n", match ? "MATCH" : "MISMATCH");
    } else {
        printf("  No matching driver found.\n");
    }
}

void menu_knn(void) {
    printf("\n--------------------------------------------------\n  OPTION 4 : K-Nearest Neighbors\n--------------------------------------------------\n");
    if (g_total_inserted == 0) { printf("  [!] No drivers in system.\n"); return; }

    double cx, cy;
    int k;
    printf("  User X : "); scanf("%lf", &cx); flush_stdin();
    printf("  User Y : "); scanf("%lf", &cy); flush_stdin();
    printf("  k (1-%d) : ", MAX_KNN); scanf("%d", &k); flush_stdin();
    if (k < 1 || k > MAX_KNN) { printf("  [!] k must be between 1 and %d.\n", MAX_KNN); return; }
    int avail;
    printf("  Only AVAILABLE drivers? [0=no / 1=yes] : "); scanf("%d", &avail); flush_stdin();

    KNNHeap heap;
    memset(&heap, 0, sizeof(heap));
    heap.k = k;

    clock_t t0 = clock();
    qt_knn(g_root, cx, cy, avail, &heap);
    double qt_time = (double)(clock() - t0) / CLOCKS_PER_SEC;
    heap_sort_ascending(&heap);

    static HeapEntry bf_out[MAX_KNN];
    t0 = clock();
    int bf_k = brute_force_knn(cx, cy, k, avail, bf_out);
    double bf_time = (double)(clock() - t0) / CLOCKS_PER_SEC;

    int correct = (heap.size == bf_k);
    for (int i = 0; i < heap.size && correct; i++)
        if (heap.entries[i].driver->id != bf_out[i].driver->id) correct = 0;

    printf("\n  k requested     : %d\n", k);
    printf("  Found           : %d\n", heap.size);
    printf("  Filter          : %s\n", avail ? "AVAILABLE" : "ALL");
    printf("  QuadTree time   : %.6f s\n", qt_time);
    printf("  BruteForce time : %.6f s\n", bf_time);
    printf("  Correctness     : %s\n", correct ? "MATCH" : "MISMATCH");

    printf("\n  %-4s  %-8s  %-9s  %-10s  %-10s  %-10s\n", "Rank", "ID", "Status", "X", "Y", "Distance");
    for (int i = 0; i < heap.size; i++) {
        Driver *d = heap.entries[i].driver;
        printf("  %-4d  %-8d  %-9s  %-10.4f  %-10.4f  %-10.4f\n",
               i+1, d->id, status_label[d->status], d->x, d->y, sqrt(heap.entries[i].dist_sq));
    }
}

void menu_gps_updates(void) {
    printf("\n--------------------------------------------------\n  OPTION 5 : Streaming GPS Updates\n--------------------------------------------------\n");
    if (g_total_inserted == 0) { printf("  [!] No drivers in system.\n"); return; }

    int n;
    printf("  How many GPS events to simulate? : "); scanf("%d", &n); flush_stdin();
    if (n <= 0) { printf("  [!] Invalid count.\n"); return; }

    clock_t t0 = clock();
    int updates = 0;
    for (int i = 0; i < n; i++) {
        int id = (rand() % g_total_inserted) + 1;
        if (id < 1 || id > MAX_DRIVERS || !IS_ACTIVE(driver_registry[id])) continue;
        double nx = driver_registry[id].x + ((double)rand()/RAND_MAX - 0.5) * 10.0;
        double ny = driver_registry[id].y + ((double)rand()/RAND_MAX - 0.5) * 10.0;
        if (nx < CITY_MIN_X) nx = CITY_MIN_X;
        if (nx > CITY_MAX_X) nx = CITY_MAX_X;
        if (ny < CITY_MIN_Y) ny = CITY_MIN_Y;
        if (ny > CITY_MAX_Y) ny = CITY_MAX_Y;
        update_driver(g_root, id, nx, ny);
        updates++;
    }
    double elapsed = (double)(clock() - t0) / CLOCKS_PER_SEC;
    printf("  Done. %d updates in %.4f s  (%.2f us/update)\n", updates, elapsed, updates > 0 ? (elapsed/updates)*1e6 : 0.0);
    print_stats(g_root);
}

void menu_status_management(void) {
    printf("\n--------------------------------------------------\n  OPTION 6 : Driver Status Management\n--------------------------------------------------\n");
    printf("  [1] View driver info\n  [2] Set driver AVAILABLE\n  [3] Set driver OFFLINE\n  [4] List all BUSY drivers\n  Choice : ");
    int choice; scanf("%d", &choice); flush_stdin();

    if (choice == 1) {
        int id;
        printf("  Driver ID : "); scanf("%d", &id); flush_stdin();
        if (id < 1 || id > MAX_DRIVERS || !IS_ACTIVE(driver_registry[id])) { printf("  [!] Driver not found.\n"); return; }
        Driver *d = &driver_registry[id];
        printf("\n  Driver #%d\n  Position    : (%.4f, %.4f)\n  Status      : %s\n  Trips done  : %d\n",
               d->id, d->x, d->y, status_label[d->status], d->trips_done);
        if (d->status == STATUS_BUSY) printf("  Current rider: #%d\n", d->current_rider);
    } else if (choice == 2) {
        int id; printf("  Driver ID : "); scanf("%d", &id); flush_stdin(); set_driver_status(id, STATUS_AVAILABLE);
    } else if (choice == 3) {
        int id; printf("  Driver ID : "); scanf("%d", &id); flush_stdin(); set_driver_status(id, STATUS_OFFLINE);
    } else if (choice == 4) {
        printf("\n  BUSY drivers:\n  %-8s  %-10s  %-10s  %-8s\n", "ID", "X", "Y", "RiderID");
        int found = 0;
        for (int i = 1; i <= MAX_DRIVERS; i++) {
            if (driver_registry[i].status == STATUS_BUSY) {
                printf("  %-8d  %-10.4f  %-10.4f  %-8d\n", driver_registry[i].id, driver_registry[i].x, driver_registry[i].y, driver_registry[i].current_rider);
                found++;
            }
        }
        if (!found) printf("  (none)\n");
    } else {
        printf("  [!] Invalid choice.\n");
    }
}

void menu_dispatch(void) {
    printf("\n--------------------------------------------------\n  OPTION 7 : Dispatch / Complete Trip\n--------------------------------------------------\n");
    printf("  [1] Dispatch nearest available driver to a rider\n  [2] Complete a trip (mark driver AVAILABLE)\n  Choice : ");
    int choice; scanf("%d", &choice); flush_stdin();

    if (choice == 1) {
        double cx, cy; int rider_id;
        printf("  Rider ID  : "); scanf("%d",  &rider_id); flush_stdin();
        printf("  Rider X   : "); scanf("%lf", &cx); flush_stdin();
        printf("  Rider Y   : "); scanf("%lf", &cy); flush_stdin();
        dispatch_nearest(g_root, cx, cy, rider_id);
    } else if (choice == 2) {
        int id; printf("  Driver ID : "); scanf("%d", &id); flush_stdin(); complete_trip(id);
    } else {
        printf("  [!] Invalid choice.\n");
    }
}

void menu_performance(void) {
    printf("\n--------------------------------------------------\n  OPTION 8 : Performance Summary\n--------------------------------------------------\n");
    if (g_total_inserted == 0) { printf("  [!] No drivers in system.\n"); return; }

    double cx = (CITY_MIN_X + CITY_MAX_X) * 0.5;
    double cy = (CITY_MIN_Y + CITY_MAX_Y) * 0.5;
    double r  = (CITY_MAX_X - CITY_MIN_X) * 0.25;
    int k = 10;

    RangeResult res = { .count = 0 };
    clock_t t0 = clock();
    qt_range_query(g_root, cx, cy, r, 1, &res);
    double qt_range = (double)(clock() - t0) / CLOCKS_PER_SEC;

    t0 = clock(); brute_force_range(cx, cy, r, 1);
    double bf_range = (double)(clock() - t0) / CLOCKS_PER_SEC;

    NNResult nn = { .driver = NULL, .dist_sq = DBL_MAX };
    t0 = clock(); qt_nearest_neighbor(g_root, cx, cy, 1, &nn);
    double qt_nn = (double)(clock() - t0) / CLOCKS_PER_SEC;

    t0 = clock(); brute_force_nn(cx, cy, 1);
    double bf_nn = (double)(clock() - t0) / CLOCKS_PER_SEC;

    KNNHeap heap; memset(&heap, 0, sizeof(heap)); heap.k = k;
    t0 = clock(); qt_knn(g_root, cx, cy, 1, &heap);
    double qt_knn_time = (double)(clock() - t0) / CLOCKS_PER_SEC;

    static HeapEntry bf_out[MAX_KNN];
    t0 = clock(); brute_force_knn(cx, cy, k, 1, bf_out);
    double bf_knn_time = (double)(clock() - t0) / CLOCKS_PER_SEC;

    double sp_range = (qt_range > 1e-9) ? bf_range / qt_range : 0;
    double sp_nn = (qt_nn > 1e-9) ? bf_nn / qt_nn : 0;
    double sp_knn = (qt_knn_time > 1e-9) ? bf_knn_time / qt_knn_time : 0;

    printf("\n  Operation           QuadTree(s)    BruteForce(s)   Speedup\n");
    printf("  Range(r=%.0f)        %-12.6f %-12.6f %.1fx\n", r, qt_range, bf_range, sp_range);
    printf("  Nearest Neighbor    %-12.6f %-12.6f %.1fx\n", qt_nn, bf_nn, sp_nn);
    printf("  K-NN(k=%d)           %-12.6f %-12.6f %.1fx\n", k, qt_knn_time, bf_knn_time, sp_knn);

    print_stats(g_root);
}
