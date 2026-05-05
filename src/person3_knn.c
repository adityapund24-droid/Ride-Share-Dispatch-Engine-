#include "ride_dispatch.h"

void heap_swap(KNNHeap *h, int a, int b) {
    HeapEntry tmp = h->entries[a];
    h->entries[a] = h->entries[b];
    h->entries[b] = tmp;
}

void heap_sift_up(KNNHeap *h, int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->entries[parent].dist_sq < h->entries[i].dist_sq) {
            heap_swap(h, parent, i);
            i = parent;
        } else break;
    }
}

void heap_sift_down(KNNHeap *h, int i) {
    while (1) {
        int largest = i;
        int l = 2*i + 1, r = 2*i + 2;
        if (l < h->size && h->entries[l].dist_sq > h->entries[largest].dist_sq) largest = l;
        if (r < h->size && h->entries[r].dist_sq > h->entries[largest].dist_sq) largest = r;
        if (largest == i) break;
        heap_swap(h, i, largest);
        i = largest;
    }
}

void heap_push(KNNHeap *h, Driver *d, double dist_sq) {
    if (h->size < h->k) {
        h->entries[h->size].dist_sq = dist_sq;
        h->entries[h->size].driver = d;
        h->size++;
        heap_sift_up(h, h->size - 1);
    } else if (dist_sq < h->entries[0].dist_sq) {
        h->entries[0].dist_sq = dist_sq;
        h->entries[0].driver = d;
        heap_sift_down(h, 0);
    }
}

double heap_worst(const KNNHeap *h) {
    if (h->size < h->k) return DBL_MAX;
    return h->entries[0].dist_sq;
}

void heap_sort_ascending(KNNHeap *h) {
    for (int i = 1; i < h->size; i++) {
        HeapEntry key = h->entries[i];
        int j = i - 1;
        while (j >= 0 && h->entries[j].dist_sq > key.dist_sq) {
            h->entries[j + 1] = h->entries[j];
            j--;
        }
        h->entries[j + 1] = key;
    }
}

void qt_knn(QuadNode *node, double cx, double cy, int available_only, KNNHeap *heap) {
    if (!node) return;
    if (bbox_min_dist_sq(&node->bbox, cx, cy) >= heap_worst(heap)) return;

    if (node->is_leaf) {
        if (node->driver) {
            if (available_only && !IS_AVAILABLE(*node->driver)) return;
            double dx = cx - node->driver->x;
            double dy = cy - node->driver->y;
            heap_push(heap, node->driver, dx*dx + dy*dy);
        }
        return;
    }

    int first = get_quadrant(&node->bbox, cx, cy);
    if (node->child[first]) qt_knn(node->child[first], cx, cy, available_only, heap);
    for (int i = 0; i < 4; i++)
        if (i != first && node->child[i]) qt_knn(node->child[i], cx, cy, available_only, heap);
}
