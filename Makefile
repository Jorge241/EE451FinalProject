CC = g++
MPICC = mpic++
DEBUG = -g -Wall
C11 = -std=c++11
FAST = -O3

all: lzw_serial_encode lzw_serial_decode lzw_parallel_encode lzw_standard_encode lzw_parallel_decode lzw_standard_encode lzw_standard_decode

lzw_serial_encode: lzw_serial_encode.cpp
	$(CC) $(FAST) $(C11) $< -o $@

lzw_serial_decode: lzw_serial_decode.cpp
	$(CC) $(FAST) $(C11) $< -o $@

lzw_parallel_encode: lzw_parallel_encode.cpp
	$(CC) $(FAST) $(C11) $< -o $@ -pthread

lzw_parallel_decode: lzw_parallel_decode.cpp
	$(CC) $(FAST) $(C11) $< -o $@ -pthread

lzw_standard_encode: lzw_standard_encode.cpp
	$(CC) $(FAST) $(C11) $< -o $@ -pthread

lzw_standard_decode: lzw_standard_decode.cpp
	$(CC) $(FAST) $(C11) $< -o $@ -pthread

input_gen: input_gen.cpp
	$(CC) $(FAST) $(C11) $< -o $@



.PHONY: clean

clean: 
	rm -f lzw_serial_encode lzw_serial_decode lzw_parallel_encode lzw_standard_encode lzw_parallel_decode lzw_standard_encode lzw_standard_decode