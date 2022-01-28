# Reshifr Multilevel Vector
# Make

ifneq (,$(filter Windows%,$(OS)))
	FILEID := rmv-$(shell PowerShell -Command "[guid]::NewGuid().ToString()")
	TARGET := v:\$(FILEID)-main
else ifneq (,$(filter Linux%,$(shell uname -s)))
	FILEID := rmv-$(shell uuidgen)
	TARGET := /tmp/$(FILEID)-main
endif

.PHONY: gcc msvc clang

gcc: main.cpp rmv.hpp
ifneq (,$(filter Windows%,$(OS)))
	g++ $< -o $(TARGET).exe -std=c++20 -O3 \
		-fopenmp -lgomp -I. -Wall -Wextra -Wshadow
	@$(TARGET).exe
	@echo Process returns value %ERRORLEVEL% . . .
	@del /F /S /Q $(TARGET).exe 1> nul 2> nul || ver > nul
else
	g++ $< -o $(TARGET) -std=c++20 -O3 \
		-fopenmp -lgomp -I. -Wall -Wextra -Wshadow
	@chmod +x $(TARGET)
	@$(TARGET)
	@echo Process returns value $$? . . .
	@rm -rf $(TARGET) &> /dev/null
endif

msvc: main.cpp rmv.hpp
ifneq (,$(filter Windows%,$(OS)))
	cl $< /std:c++20 /O2 /EHsc /I. /W4 \
		/Fe: $(TARGET).exe \
		/Fo: $(TARGET).obj
	@$(TARGET).exe
	@echo Process returns value %ERRORLEVEL% . . .
	@del /F /S /Q $(TARGET).exe $(TARGET).obj 1> nul 2> nul || ver > nul
else
	@echo Unsupported platform . . .
endif

clang: main.cpp rmv.hpp
ifneq (,$(filter Windows%,$(OS)))
	clang++ $< -o $(TARGET).exe -std=c++20 -O3 \
		-I. -Wall -Wextra -Wshadow
	@$(TARGET).exe
	@echo Process returns value %ERRORLEVEL% . . .
	@del /F /S /Q $(TARGET).exe 1> nul 2> nul || ver > nul
else
	clang++ $< -o $(TARGET) -std=c++20 -O3 \
		-I. -Wall -Wextra -Wshadow
	@chmod +x $(TARGET)
	@$(TARGET)
	@echo Process returns value $$? . . .
	@rm -rf $(TARGET) &> /dev/null
endif

nvc: main.cpp rmv.hpp
ifneq (,$(filter Windows%,$(OS)))
	@echo Unsupported platform . . .
else
	nvc++ $< -o $(TARGET) -march=native -std=c++20 -O4 \
		-I. -Wall -Wextra -Wshadow
	@chmod +x $(TARGET)
	@$(TARGET)
	@echo Process returns value $$? . . .
	@rm -rf $(TARGET) &> /dev/null
endif

memcheck: main.cpp rmv.hpp
ifneq (,$(filter Windows%,$(OS)))
	@echo Unsupported platform . . .
else ifneq (,$(filter Linux%,$(shell uname -s)))
	g++ -ggdb3 $< -o $(TARGET) -O3 -std=c++20 -w -I.
	@chmod +x $(TARGET)
	@valgrind \
		--tool=memcheck \
		--leak-check=full \
		--leak-resolution=high \
		--leak-check-heuristics=all \
		--show-leak-kinds=all \
		--track-origins=yes $(TARGET)
	@echo Process returns value $$? . . .
	@rm -rf $(TARGET) &> /dev/null
endif

massif: main.cpp rmv.hpp
ifneq (,$(filter Windows%,$(OS)))
	@echo Unsupported platform . . .
else ifneq (,$(filter Linux%,$(shell uname -s)))
	@g++ -ggdb3 $< -o $(TARGET) -O3 -std=c++20 -w -I.
	@chmod +x $(TARGET)
	@valgrind \
		--tool=massif \
		--depth=200 \
		--time-unit=ms \
		--detailed-freq=1 \
		--max-snapshots=1000 \
		--massif-out-file=$(TARGET).massif \
		--log-file=/dev/null $(TARGET) > /dev/null
	@rm -rf $(TARGET) &> /dev/null
	@massif-visualizer $(TARGET).massif
	@rm -rf $(TARGET).massif &> /dev/null
endif
