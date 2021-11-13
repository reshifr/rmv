# Reshifr Multilevel Vector
# Make

CC = gcc
CXX = g++

CFLAGS += -std=c99 -O3
CXXFLAGS += -std=c++11 -O3

OLD = old
MAIN = main

ifneq (,$(filter Windows%,$(OS)))
	OLD := $(OLD).exe
	MAIN := $(MAIN).exe
endif

.PHONY: old main

all: old main

old: old.c
	$(CC) $< -o $@ $(CFLAGS)
ifneq (,$(filter Windows%,$(OS)))
	@$(OLD)
	@del /F /S /Q $(OLD) 1> nul 2> nul || ver > nul
else
	@chmod +x $(OLD)
	@./$(OLD)
	@rm -rf $(OLD) &> /dev/null
endif

main: main.cpp rmv.h
	$(CXX) $< -o $@ $(CXXFLAGS)
ifneq (,$(filter Windows%,$(OS)))
	@$(MAIN)
	@del /F /S /Q $(MAIN) 1> nul 2> nul || ver > nul
else
	@chmod +x $(MAIN)
	@./$(MAIN)
	@rm -rf $(MAIN) &> /dev/null
endif
