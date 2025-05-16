SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
TARGET := libUnBufferedChannel.a

$(TARGET): $(OBJS)
	@echo "Build Mode $(BuildMode)"
	@echo "Building UnBufferedChannel"
	# ar -rcs $@ $^

%.o: %.cpp
	g++ -std=c++17 -c -I. $< -o $@ 


clean:
	rm -rf *.a & rm -rf *.o & rm -rf $(TARGET)