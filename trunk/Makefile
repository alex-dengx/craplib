SRC_DIRS = .
CPPFILES = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
OBJS =  $(CPPFILES:.cpp=.o)

CC = g++
CPPFLAGS = -std=gnu++98 -pedantic -DDEBUG

AR = ar
ARFLAGS = rcs

all: $(OBJS) lib tests

%.o: %.cpp
	@echo "[+] Compile a component: $<"
	@$(CC) -c $(CPPFLAGS) $< -o $(<:.cpp=.o) 2> /dev/null

lib:
	@$(AR) $(ARFLAGS) libcrap.a $(OBJS)

tests:
	@cd Tests && make

clean:
	@rm *.o
	@cd Tests && make clean
