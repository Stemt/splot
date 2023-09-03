#!/usr/bin/bash
./ssplit "./splot -x 0 -y 0 -w 920 -h 500 -n Signal_1" \
	"./splot -x 960 -y 0 -w 920 -h 500 -n Signal_2" \
	"./splot -x 0 -y 540 -w 920 -h 500 -n Signal_3" \
	"./splot -x 960 -y 540 -w 920 -h 500 -n Signal_4" 
