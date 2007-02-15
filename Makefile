CFLAGS = -O2 -Wall -Wno-parentheses -Wno-switch -Wstrict-prototypes -Werror-implicit-function-declaration -Wpointer-arith \
	-Wmissing-prototypes -Wmissing-declarations -Wcomments -Wextra -Wall -pedantic -ansi

all: shownum

shownum: shownum.c
	gcc $(CFLAGS) -s $< -o $@

install: shownum
	cp shownum /usr/local/bin/shownum

clean:
	rm -f shownum

release: clean
	mkdir -p shownum
	cp shownum.c LICENSE README Makefile shownum
	tar -czvf shownum.tar.gz shownum
	rm -rf shownum
	@echo "upload shownum.tar.gz somewhere.."
