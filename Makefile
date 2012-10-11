all : atest attrib

atest : attrib.c atest.c
	gcc -g -Wall -o $@ $^

attrib : attrib.c lua-attrib.c
	gcc -g -Wall --shared -o $@.dll $^ -I/usr/local/include -L/usr/local/bin -llua52
