SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

LIBS_PATH := ../..
INCLUDES := -I$(LIBS_PATH)/ThreadPools \
-I$(LIBS_PATH)/Channels \
-I$(LIBS_PATH)/Channels/ChannelSelector \
-I$(LIBS_PATH)/Channels/BufferedChannel \
-I$(LIBS_PATH)/Channels/UnBufferedChannel \
-I$(LIBS_PATH)/Thread \

TARGET := BasicThreadPoolUserware.exe

all: $(TARGET)

$(TARGET): $(OBJS) LIBS_BUILD
	g++ -std=c++17 $(OBJS) $(LIBS_PATH)/Thread/*.o -o $@

LIBS_BUILD:
	make -j -C $(LIBS_PATH)/Thread

%.o: %.cpp
	g++ -std=c++17  -g -O0 $(INCLUDES) -MMD -MP  -c $< -o $@ 

clean: clean_lib clean_userware
	

clean_userware:
	rm -rf $(TARGET) *.o *.d

clean_lib:
	make clean -C $(LIBS_PATH)/Thread


-include $(DEPS)