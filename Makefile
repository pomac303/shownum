CFLAGS = -O2 -Wall -Wno-parentheses -Wno-switch -Wstrict-prototypes \
	-Werror-implicit-function-declaration -Wpointer-arith -Wmissing-prototypes \
	-Wmissing-declarations -Wcomments -Wextra -Wall

all: shownum

shownum: shownum.c
	gcc $(CFLAGS) -s $< -o $@

test: shownum.c
	gcc -ggdb $(CFLAGS) -D TEST_PARSERS -s $< -o $@
	@./test

install: shownum
	cp shownum /usr/local/bin/shownum

clean:
	rm -f shownum
	rm -f test
	rm -f *~
	rm -f *.tar.gz

release:  test clean
	mkdir -p shownum
	cp shownum.c LICENSE README Makefile shownum
	tar -czvf shownum.tar.gz shownum
	rm -rf shownum

