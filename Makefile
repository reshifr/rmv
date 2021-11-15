# Reshifr Multilevel Vector
# Make

CC = gcc
CXX = g++

CFLAGS += -std=c99 -O3
CXXFLAGS += -std=c++11 -O3 -fopenmp -lgomp

OLD = old
MAIN = main

UNAME = $(shell uname -s)

ifneq (,$(filter Linux%,$(UNAME)))
	OLD := /mnt/Volatile/$(OLD)
	MAIN := /mnt/Volatile/$(MAIN)
endif

ifneq (,$(filter Windows%,$(OS)))
	OLD := V:\$(OLD).exe
	MAIN := V:\$(MAIN).exe
endif

.PHONY: old main

all: old main

old: old.c
	$(CC) $< -o $(OLD) $(CFLAGS)
ifneq (,$(filter Windows%,$(OS)))
	@$(OLD)
	@del /F /S /Q $(OLD) 1> nul 2> nul || ver > nul
else
	@chmod +x $(OLD)
	@$(OLD)
	@rm -rf $(OLD) &> /dev/null
endif

main: main.cpp rmv.h
	$(CXX) $< -o $(MAIN) $(CXXFLAGS)
ifneq (,$(filter Windows%,$(OS)))
	@$(MAIN)
	@del /F /S /Q $(MAIN) 1> nul 2> nul || ver > nul
else
	@chmod +x $(MAIN)
	@$(MAIN)
	@rm -rf $(MAIN) &> /dev/null
endif
