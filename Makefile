all: 
	$(CXX) ./src/*.cc ./test/z80test.cc -I ./include -D__Z80TEST__ -D__Z80MEMCALLBACKS__ -std=c++11 -W -Wall -Wextra -Winline -pedantic -pedantic-errors -m64 -O3 -funroll-loops -fomit-frame-pointer -o z80test
	$(CXX) ./src/*.cc ./test/zextest.cc -I ./include -D__Z80TEST__ -std=c++11 -W -Wall -Wextra -Winline -pedantic -pedantic-errors -m64 -O3 -funroll-loops -fomit-frame-pointer -o zextest
