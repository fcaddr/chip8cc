CXX = g++
SDL_LIB = -L/usr/lib/x86_64-linux-gnu -lSDL2
SDL_INCLUDE = -I/usr/include
CXXFLAGS = -Wall -c -std=c++11 $(SDL_INCLUDE)
LDFLAGS = $(SDL_LIB)
EXE = chip8cc
SRC_FILES = $(wildcard *.cc)
HEADER_FILES = $(wildcard *.h)
OBJ_FILES = $(patsubst %.cc, %.o, $(SRC_FILES))

all: $(EXE)

$(EXE): $(OBJ_FILES) $(HEADER_FILES)
	$(CXX) $^ $(LDFLAGS) -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -o $@ $<
 
clean:
	rm *.o && rm $(EXE)