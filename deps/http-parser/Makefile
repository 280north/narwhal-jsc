
include ../../config.mk

OPT=-O0 -g -Wall -Wextra #-force_cpusubtype_ALL -mmacosx-version-min=10.4 -arch i386 #-Werror
# OPT=-O2

OPT=-O0 -g -Wall -Wextra ${CFLAGS} ${SO_CFLAGS}

test: http_parser.o test.c
	gcc $(OPT) http_parser.o test.c -o $@

http_parser.o: http_parser.c http_parser.h Makefile
	gcc $(OPT) -c http_parser.c

# HACK: made this task manual since it was sometimes being run even if ragel wasn't installed, causing build to fail.
# http_parser.c: http_parser.rl Makefile
rerun-ragel:
	ragel -s -G2 http_parser.rl -o $@

tags: http_parser.rl http_parser.h test.c
	ctags $^

clean:
	rm -f *.o test http_parser.tar

package: http_parser.c
	@rm -rf /tmp/http_parser && mkdir /tmp/http_parser && \
	cp LICENSE README.md Makefile http_parser.c http_parser.rl \
		http_parser.h test.c /tmp/http_parser && \
	cd /tmp && \
	tar -cf http_parser.tar http_parser/
	@echo /tmp/http_parser.tar

.PHONY: clean package
