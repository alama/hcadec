CXXFLAGS+=-Wall
CXXFLAGS+=-W
CXXFLAGS+=-Wno-strict-aliasing
CXXFLAGS+=-Werror
CXXFLAGS:=-Og -g -march=native -msse2 -mfpmath=sse $(CXXFLAGS)
PKGCONFIG:=pkg-config
#Need g++ 4.8+
CPPFLAGS+=-std=c++11
ifndef NOSND
CPPFLAGS+=$(shell $(PKGCONFIG) sndfile --cflags)
CPPFLAGS+=-DHAVE_SNDFILE
LDFLAGS+=$(shell $(PKGCONFIG) sndfile --libs)
endif
LDFLAGS+=-g

SRCS=clHCA.cpp  main.cpp  Path.cpp

OBJS=$(subst .cpp,.o,$(SRCS))

EXE=hcadec

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS) $(EXE)

dist-clean: clean
	$(RM) *~ .depend

include .depend
