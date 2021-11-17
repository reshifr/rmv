# Reshifr Multilevel Vector
# Make

ifneq (,$(filter Windows%,$(OS)))
	TARGET := V:\main
else ifneq (,$(filter Linux%,$(shell uname -s)))
	TARGET := /mnt/Volatile/main
endif

.PHONY: gcc msvc clang

gcc: main.cpp rmv.hpp
ifneq (,$(filter Windows%,$(OS)))
	g++ $< -o $(TARGET).exe -std=c++11 -O3 -fopenmp -lgomp -I.
	@$(TARGET).exe
	@echo Process returns value %ERRORLEVEL% . . .
	@del /F /S /Q $(TARGET).exe 1> nul 2> nul || ver > nul
else
	g++ $< -o $(TARGET) -std=c++11 -O3 -fopenmp -lgomp -I.
	@chmod +x $(TARGET)
	@$(TARGET)
	@rm -rf $(TARGET) &> /dev/null
endif

msvc: main.cpp rmv.hpp
	cl $< /std:c++17 /O2 /EHsc /I. /Fe:$(TARGET).exe /Fo:$(TARGET).obj
	@$(TARGET).exe
	@echo Process returns value %ERRORLEVEL% . . .
	@del /F /S /Q $(TARGET).exe $(TARGET).obj 1> nul 2> nul || ver > nul

clang: main.cpp rmv.hpp
ifneq (,$(filter Windows%,$(OS)))
	clang++ $< -o $(TARGET).exe -std=c++17 -O3 -I.
	@$(TARGET).exe
	@echo Process returns value %ERRORLEVEL% . . .
	@del /F /S /Q $(TARGET).exe 1> nul 2> nul || ver > nul
else
	clang++ $< -o $(TARGET) -std=c++11 -O3 -I.
	@chmod +x $(TARGET)
	@$(TARGET)
	@rm -rf $(TARGET) &> /dev/null
endif
