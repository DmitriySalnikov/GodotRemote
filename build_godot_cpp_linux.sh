#!/usr/bin/env bash
cd godot-cpp
#api=custom_api_file="../api.json"
api= 

scons platform=linux target=release bits=64 $api
scons platform=linux target=debug bits=64 $api
