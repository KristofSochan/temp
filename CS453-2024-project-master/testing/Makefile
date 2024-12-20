CXX = g++
CXXFLAGS = -Wall -g -I../include -fsanitize=address,undefined -std=c++17
LDFLAGS = -L../394729 -l394729 -ldl -lpthread

TARGET = main
SRCS = main.cpp
LIBRARY_SRC = ../394729/*.cpp
LIBRARY = ../394729/lib394729.so

OBJS = $(SRCS:.cpp=.o)

# First, compile the shared library from 394729
$(LIBRARY): $(LIBRARY_SRC)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^

all: $(TARGET)

$(TARGET): $(OBJS) $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	LD_LIBRARY_PATH=../394729 ./$(TARGET)

debug: $(TARGET)
	LD_LIBRARY_PATH=../394729 gdb ./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) $(LIBRARY)

.PHONY: all clean run debug
