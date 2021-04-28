#!/usr/bin/env bash
cd godot-cpp
cpu=$(grep ^cpu\\scores /proc/cpuinfo | uniq |  awk '{print $4}')
api=custom_api_file="../api.json"

scons platform=linux target=release bits=64 -j$cpu $api
scons platform=linux target=debug bits=64 -j$cpu $api