# Compilers
CXX = g++

# Flags
CXXFLAGS = -std=c++17 -Wall -Isrc -Iinterface
LDFLAGS = -lm

TARGET = a.out

SOURCES = main.cpp \
		  src/driver_bmp280.c \
		  interface/driver_bmp280_interface.c

OBJECTS = $(SOURCES:.cpp=.o)
OBJECTS := $(OBJECTS:.c=.o)

$(TARGET) : $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)