CXXFLAGS+=-Wall
CXXFLAGS+=-W
#CXXFLAGS+=-Wno-strict-aliasing
CXXFLAGS+=-Werror
ifdef DEBUG
CXXFLAGS:=-Og -g -march=native $(CXXFLAGS)
else
CXXFLAGS:=-O3 -s -march=native $(CXXFLAGS)
endif
ifdef PROFILE
CXXFLAGS+= -pg
endif
PKGCONFIG:=pkg-config
#Need g++ 4.8+
CPPFLAGS+=-std=c++11
ifndef NOSND
CPPFLAGS+=$(shell $(PKGCONFIG) sndfile --cflags)
CPPFLAGS+=-DHAVE_SNDFILE
LDFLAGS+=$(shell $(PKGCONFIG) sndfile --libs)
endif
ifdef DEBUG
LDFLAGS+=-g
else
LDFLAGS+=-s
endif
ifdef PROFILE
LDFLAGS+=-pg
endif

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
