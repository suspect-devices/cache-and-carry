#!/bin/bash

LIB_MAPLE_HOME=`dirname ..` USER_MODULES=`pwd` BOARD=maple_mini make -d -f ../Makefile $@
