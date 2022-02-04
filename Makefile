CXXFLAGS=-g # -g = debug, -O2 for optimized code CXXFLAGS for g++
CC=g++

linker: main2.cpp tokenizer.cpp
$(CC) $(CXXFLAGS) –o linker main2.cpp tokenizer.cpp

clean:
	  rm -f linker *~