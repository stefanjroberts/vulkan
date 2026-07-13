#!/bin/bash

compile_targets=$(find . -type f -name "*.cpp")

link_flags=" -lglfw -lvulkan"

g++ -o bin/main -g $compile_targets $link_flags
