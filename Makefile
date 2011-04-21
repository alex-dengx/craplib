SRC_DIRS = .
CPPFILES = $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
OBJS =  $(CPPFILES:.cpp=.o)

CC = g++
CPPFLAGS = -pedantic -DDEBUG

AR = ar
ARFLAGS = rcs

all: $(OBJS) lib tests

%.o: %.cpp
	@echo "[+] Compile a component: $<"
	$(CC) -c $(CPPFLAGS) $< -o $(<:.cpp=.o)

lib:
	@$(AR) $(ARFLAGS) libcrap.a $(OBJS)

tests:
	@cd Tests && make

clean:
	@rm *.o
	@cd Tests && make clean
