
CONCURRENCY_LIB_PATH = ./

# this depends on the library being named Concurrency-Lib
include $(CONCURRENCY_LIB_PATH)/Common.mk

all: Thread-Lib Actors-Lib

Thread-Lib:
	make -j -C $(THREAD_PATH)

Actors-Lib:
	make -j -C $(ACTORS_PATH)


clean:
	make clean -C $(ACTORS_PATH)
	make clean -C $(THREAD_PATH)
