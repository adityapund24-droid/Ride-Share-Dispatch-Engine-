@echo off
REM Compile and run Ride Dispatch project on Windows with MinGW GCC
gcc -Wall -Wextra -std=c11 -Iinclude -o ride_dispatch.exe main.c src/globals.c src/person1_core.c src/person2_spatial_queries.c src/person3_knn.c src/person4_status_dispatch.c src/person5_diagnostics_performance.c src/menu.c -lm
if %errorlevel% neq 0 (
    echo Build failed.
    pause
    exit /b %errorlevel%
)
ride_dispatch.exe
pause
