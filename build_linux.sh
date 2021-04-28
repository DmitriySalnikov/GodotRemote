#!/usr/bin/env bash
cpu=$(grep ^cpu\\scores /proc/cpuinfo | uniq |  awk '{print $4}')

scons platform=linux target=release bits=64 -j$cpu
scons platform=linux target=debug bits=64 -j$cpu
