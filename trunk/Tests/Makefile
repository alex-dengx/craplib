CC = g++
CPPFLAGS = -pedantic -DDEBUG

all:
	@echo -e "Compiling Tests..."
	@$(CC) $(CPPFLAGS) -o Tests Tests.cpp -L.. -I.. -lcrap -lpthread

clean:
	@rm Tests
