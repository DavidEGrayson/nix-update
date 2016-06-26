DESTDIR ?= /usr/local

all:
	g++ -O3 -Wall --std=c++11 -o nix-update-git \
          $(foreach n,$(nativeBuildInputs),-I$n/include/nix) \
	  $(NIX_CFLAGS_COMPILE) $(foreach f,$(NIX_LDFLAGS),-Wl,$f) \
	  nix-update-git.cc \
	  -lnixmain -lnixexpr

install:
	mkdir -p $(DESTDIR)/bin
	cp nix-update-git $(DESTDIR)/bin
