#!/bin/bash

mkdir -p build
cd build
cmake -G Ninja -DFEATURE_sql=OFF -DFEATURE_gui=OFF -DCMAKE_BUILD_TYPE=Debug -DFEATURE_sanitize_thread=ON -DFEATURE_shared=ON -DFEATURE_static=OFF -DFEATURE_rpath=OFF -DFEATURE_dbus=OFF -DFEATURE_testlib=OFF  -DFEATURE_thread=ON .. 
cmake --build . -j 8
./hello_world_app