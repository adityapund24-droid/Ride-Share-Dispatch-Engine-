#include "ride_dispatch.h"

int main(void) {
    srand((unsigned)time(NULL));

    BBox city = { CITY_MIN_X, CITY_MIN_Y, CITY_MAX_X, CITY_MAX_Y };
    g_root = create_node(city);

    memset(driver_registry, 0, sizeof(driver_registry));
    for (int i = 0; i <= MAX_DRIVERS; i++) {
        driver_registry[i].status = STATUS_INACTIVE;
        driver_registry[i].current_rider = -1;
    }

    int choice;
    do {
        print_menu();
        if (scanf("%d", &choice) != 1) { flush_stdin(); continue; }
        flush_stdin();

        switch (choice) {
            case 1: menu_insert(); break;
            case 2: menu_range_query(); break;
            case 3: menu_nearest_neighbor(); break;
            case 4: menu_knn(); break;
            case 5: menu_gps_updates(); break;
            case 6: menu_status_management(); break;
            case 7: menu_dispatch(); break;
            case 8: menu_performance(); break;
            case 0: printf("\n  Goodbye.\n"); break;
            default: printf("  [!] Invalid choice.\n");
        }
    } while (choice != 0);

    free_tree(g_root);
    return 0;
}
