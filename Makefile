DESTDIR ?= /usr/local

CFLAGS += -O3 -Wall --std=c++11
CFLAGS += $(foreach n,$(nativeBuildInputs),-I$n/include/nix)
CFLAGS += $(NIX_CFLAGS_COMPILE)

LDFLAGS += $(foreach f,$(NIX_LDFLAGS),-Wl,$f)

all:
	g++ -c -o libupdate.o $(CFLAGS) libupdate.cc
	g++ -o nix-update-git $(CFLAGS) $(LDFLAGS) \
	  nix-update-git.cc libupdate.o \
          -lnixmain -lnixexpr

install:
	mkdir -p $(DESTDIR)/bin
	cp nix-update-git $(DESTDIR)/bin
