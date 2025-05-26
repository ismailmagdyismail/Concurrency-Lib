CXX := g++
CXXFLAGS := -Wall -Wextra -g -O0 -std=c++17 -MMD -MP

# CONCURRENCY_LIB_PATH is defined by the includer of this make file

THREAD_PATH := $(CONCURRENCY_LIB_PATH)/Thread
CHANNELS_PATH := $(CONCURRENCY_LIB_PATH)/Channels/
UnBufferedChannelPath := $(CONCURRENCY_LIB_PATH)/Channels/UnBufferedChannel
BufferedChannelPath := $(CONCURRENCY_LIB_PATH)/Channels/BufferedChannel
ChannelSelectorPath := $(CONCURRENCY_LIB_PATH)/Channels/ChannelSelector
ACTORS_PATH = $(CONCURRENCY_LIB_PATH)/Actors/


CONCURRENCY_LIB_INCLUDES := -I$(CHANNELS_PATH) \
-I$(UnBufferedChannelPath)\
-I$(THREAD_PATH) \
-I$(BufferedChannelPath) \
-I$(ChannelSelectorPath) \
-I$(ACTORS_PATH) \
