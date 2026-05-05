# Ride Dispatch Engine using QuadTree

This ADS project is divided member-wise into separate C files.

## Folder Structure

```text
Ride_dispatch_project/
+-- include/
|   +-- ride_dispatch.h
+-- src/
|   +-- globals.c
|   +-- person1_core.c
|   +-- person2_spatial_queries.c
|   +-- person3_knn.c
|   +-- person4_status_dispatch.c
|   +-- person5_diagnostics_performance.c
|   +-- menu.c
+-- main.c
+-- Makefile
+-- README.md
```

## Work Division

### Person 1 - Core Data Structures & Tree Mechanics
File: `src/person1_core.c`

Responsibilities:
- Bounding box helper functions
- Quadrant calculation
- Child bounding box generation
- QuadTree node creation
- Insert driver
- Delete driver
- Update driver GPS location
- Free QuadTree memory

### Person 2 - Spatial Queries
File: `src/person2_spatial_queries.c`

Responsibilities:
- Range query inside radius
- Single nearest-neighbor query
- Branch-and-bound pruning logic

### Person 3 - K-NN
File: `src/person3_knn.c`

Responsibilities:
- Fixed-size max heap
- Heap insert and sort
- K-nearest-neighbor search
- K-NN pruning using current kth best distance

### Person 4 - Driver Status & Dispatch Logic
File: `src/person4_status_dispatch.c`

Responsibilities:
- Driver status transitions
- Available / Busy / Offline management
- Dispatch nearest available driver
- Complete trip logic

### Person 5 - Diagnostics, Brute Force & Performance
File: `src/person5_diagnostics_performance.c`

Responsibilities:
- QuadTree statistics
- Brute force nearest neighbor
- Brute force range query
- Brute force K-NN
- Correctness verification support

### Main Menu / UI
File: `src/menu.c`

Responsibilities:
- Console menu
- User input
- Calling each member's module functions

### Program Entry
File: `main.c`

Responsibilities:
- Initialize city QuadTree
- Initialize driver registry
- Run main menu loop
- Free memory before exit

## How to Compile

### Linux / macOS / Git Bash / WSL

```bash
make
./ride_dispatch
```

### Windows PowerShell with GCC

```powershell
gcc -Wall -Wextra -std=c11 -Iinclude -o ride_dispatch.exe main.c src/*.c -lm
.\ride_dispatch.exe
```

If `src/*.c` does not expand in your PowerShell, use:

```powershell
gcc -Wall -Wextra -std=c11 -Iinclude -o ride_dispatch.exe main.c src/globals.c src/person1_core.c src/person2_spatial_queries.c src/person3_knn.c src/person4_status_dispatch.c src/person5_diagnostics_performance.c src/menu.c -lm
.\ride_dispatch.exe
```

## Suggested Demo Flow

1. Select option `1`
2. Select `1` for auto-generate drivers
3. Enter `10000`
4. Use option `2` for range query
5. Use option `3` for nearest driver
6. Use option `4` for K-nearest drivers
7. Use option `7` to dispatch a driver
8. Use option `8` for performance summary
