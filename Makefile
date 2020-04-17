CXX=g++
CXXFLAGS=-Wall -Wextra -pedantic -std=c++17 -O3
LDFLAGS=$(CXXFLAGS)
LIBS=-pthread -lboost_system -lboost_program_options
OBJ=$(SRC:.cc=.o)

all:  cache_server test_cache_lib test_cache_client test_evictors test_workload

cache_server: cache_server.o cache_lib.o lru_evictor.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_evictors: test_evictors.o lru_evictor.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_workload: test_generate_workload.o cache_client.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_cache_lib: test_cache_lib.o cache_lib.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

test_cache_client: test_cache_client.o cache_client.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cc %.hh
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c -o $@ $<

clean:
	rm -rf *.o test_cache_client test_cache_lib test_evictors cache_server test_workload

test: all
	./test_cache_lib
	./test_evictors
	echo "test_cache_client must be run manually against a running server"

valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all ./test_cache_lib
	valgrind --leak-check=full --show-leak-kinds=all ./test_evictors
