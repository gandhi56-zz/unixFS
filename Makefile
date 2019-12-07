CC = g++
CCFLAGS = -Wall -Werror -std=c++11
BINNAME = fs

.PHONY: clean compile
compile:
	@$(CC) $(CCFLAGS) FileSystem.cpp -o $(BINNAME)

run: $(BINNAME)
	@./$(BINNAME)

clean:
	@rm -rf $(BINNAME)

compress:
	tar -caf mapreduce.tar.gz FileSystem.cpp FileSystem.h Makefile readme.md
