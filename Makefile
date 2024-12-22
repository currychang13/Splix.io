include ../Make.defines
PROGS = splix_client 
.PHONY: all clean

all:$(PROGS)

splix_client: splix_client.cpp
	g++ $(CFLAGS) -o $@ $< $(LIBS) -lncurses
clean:
	rm -f $(PROGS) $(CLEANFILES)