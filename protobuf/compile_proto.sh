#!/bin/bash

protoc -I=./ --cpp_out=./ *.proto
