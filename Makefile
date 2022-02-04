CXXFLAGS=-g # -g = debug, -O2 for optimized code CXXFLAGS for g++
CC=g++

linker: main2.cpp tokenizer.cpp
        g++ –o linker main2.cpp tokenizer.cpp -std=c++11

clean: rm -f linker *~