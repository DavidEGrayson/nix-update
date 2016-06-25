all:
	g++ -Wall --std=c++0x -I/nix/store/rqnhpzgfi1g23fcph5dcip2dqhdn66b7-nix-1.11.2-dev/include/nix $(NIX_CFLAGS_COMPILE) $(foreach f,$(NIX_LDFLAGS),-Wl,$f) test.cc -lnixformat -lnixutil -lnixstore -lnixexpr -lnixmain -lnixexpr
