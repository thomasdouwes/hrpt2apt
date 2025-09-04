cmake_minimum_required(VERSION 3.12)
project(hrp2apt)

file(GLOB_RECURSE hrpt2apt_src src/*.c)
add_executable(hrpt2apt ${hrpt2apt_src})