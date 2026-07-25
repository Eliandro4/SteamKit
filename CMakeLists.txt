cmake_minimum_required(VERSION 3.16)
project(SteamKit C)

# Use C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

enable_testing()

# Add subdirectory
add_subdirectory(steamkit_c)
