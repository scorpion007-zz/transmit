#!/bin/bash

tar -C rel -czvf transmit-`git describe`.tgz transmit.exe
