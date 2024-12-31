PROGS = splix_client splix_client.o server
.PHONY: all clean

all:$(PROGS)

splix_client: splix_client.o splix_util.o
	g++ -o $@ $^ $(LIBS) -lncursesw

splix_util.o: splix_util.cpp splix_header.h
	g++ $(CFLAGS) -I. -c $< -o $@ 

splix_client.o: splix_client.cpp splix_header.h 
	g++ $(CFLAGS) -I. -c $< -o $@ 
server: server.cpp
	g++ -o $@ $^ 
clean:
	rm -f $(PROGS) $(CLEANFILES)