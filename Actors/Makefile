SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

CXX := g++
CXXFLAGS := -Wall -Wextra -g -O0 -std=c++17 -MMD -MP

LIB_ROOT = ../

INCLUDES = -I.\
-I$(LIB_ROOT)/Channels/UnBufferedChannel \
-I$(LIB_ROOT)/Channels/BufferedChannel \
-I$(LIB_ROOT)/Channels/ \
-I$(LIB_ROOT)/Thread \


all: $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf *.a *.o *.d $(TARGET)

-include $(DEPS)
