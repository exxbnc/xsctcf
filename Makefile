CC ?= gcc
CFLAGS ?= -Wall -Wextra -Werror -pedantic -std=gnu99 -O2 -I /usr/X11R6/include
LDFLAGS ?= -L /usr/X11R6/lib -s
PREFIX ?= /usr
BIN ?= $(PREFIX)/bin
INSTALL ?= install

PROG = xsctcf
SRCS = src/xsctcf.c

CDIR = $(HOME)/.config/xsctcf
CNAME = xsctcf.conf
SCONF = src/example_xsctcf.conf

LIBS = -lX11 -lXrandr -lm

$(PROG): $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

install: $(PROG) $(PROG).1
	$(INSTALL) -d $(DESTDIR)$(BIN)
	$(INSTALL) -m 0755 $(PROG) $(DESTDIR)$(BIN)

uninstall:
	rm -f $(BIN)/$(PROG)

config:
	@if [ ! -d "$(CDIR)" ]; then \
		mkdir "$(CDIR)"; \
	fi

	@if [ ! -f "$(CDIR)/$(CNAME)" ]; then \
		cp "$(SCONF)" "$(CDIR)/$(CNAME)"; \
	fi

clean:
	rm -f $(PROG)
