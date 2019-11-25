CC = g++
CCFLAGS = -Wall -Werror -pthread -std=c++11
BINNAME = fs

.PHONY: clean compile
compile:
	@$(CC) $(CCFLAGS) FileSystem.cpp -o $(BINNAME)
	cp $(BINNAME) ./sample_test_1/
	cp $(BINNAME) ./sample_test_2/
	cp $(BINNAME) ./sample_test_3/
	cp $(BINNAME) ./sample_test_4/

run: $(BINNAME)
	@./$(BINNAME)

clean:
	@rm -rf $(BINNAME)

compress:
	tar -caf mapreduce.tar.gz FileSystem.cpp FileSystem.h Makefile README.md
