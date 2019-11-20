CC = g++
CCFLAGS = -Wall -Werror -pthread -std=c++17
BINNAME = fs

.PHONY: clean compile
compile:
	@$(CC) $(CCFLAGS) FileSystem.cpp -o fs

run: $(BINNAME)
	@./$(BINNAME)

clean:
	@rm -rf $(BINNAME)

compress:
	tar -caf mapreduce.tar.gz FileSystem.cpp FileSystem.h Makefile README.md
