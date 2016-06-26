DESTDIR ?= /usr/local

all:
	g++ -Wall --std=c++0x -o update-git \
          $(foreach n,$(nativeBuildInputs),-I$n/include/nix) \
	  $(NIX_CFLAGS_COMPILE) $(foreach f,$(NIX_LDFLAGS),-Wl,$f) \
	  $(wildcard *.cc) \
	  -lnixformat -lnixutil -lnixstore -lnixmain -lnixexpr

install:
	mkdir -p $(DESTDIR)/bin
	cp update-git $(DESTDIR)/bin
