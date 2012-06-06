CFLAGS = -O2 -Wall -Wno-parentheses -Wno-switch -Wstrict-prototypes \
	-Werror-implicit-function-declaration -Wpointer-arith -Wmissing-prototypes \
	-Wmissing-declarations -Wcomments -Wextra -Werror

# Splint flags:
# +matchanyintegral => we define our own types.
# -initallelements  => splint doesn't know about = {0}
# -mustfreeonly     => we don't allocate any memory, it's all on stack.
SFLAGS = +matchanyintegral -initallelements -mustfreeonly \
	-predboolint -formatconst -formatcode +quiet

all: shownum

shownum: shownum.c
	@echo "Building..."
	@gcc $(CFLAGS) -s $< -o $@

check: shownum.c
	@echo "Checking with splint..."
	@splint $(SFLAGS) $< 

test: shownum.c
	@echo "Compiling test binary..."
	@gcc -ggdb $(CFLAGS) -D TEST_PARSERS -s $< -o $@
	@echo "Testing..."
	@./test

install: shownum
	cp shownum /usr/local/bin/shownum

clean:
	@echo "Cleaning up..."
	@rm -f shownum
	@rm -f test
	@rm -f *~
	@rm -f *.tar.gz

release:	check test clean
	@echo "Creating archive..."
	@mkdir -p shownum
	@cp shownum.c LICENSE README Makefile shownum
	@tar -czf shownum.tar.gz shownum
	@rm -rf shownum

