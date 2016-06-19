CXXFLAGS+=-Wall
CXXFLAGS+=-W
CXXFLAGS+=-Werror
ifdef PROFILE
CXXFLAGS+= -pg -g
endif
ifdef DEBUG
CXXFLAGS:=-Og -ggdb -march=native $(CXXFLAGS)
else
CXXFLAGS:=-O3 -s -march=native $(CXXFLAGS)
endif
PKGCONFIG:=pkg-config
#Need g++ 4.8+
CPPFLAGS+=-std=c++11
ifndef NOSND
CPPFLAGS+=$(shell $(PKGCONFIG) sndfile --cflags)
CPPFLAGS+=-DHAVE_SNDFILE
LDFLAGS+=$(shell $(PKGCONFIG) sndfile --libs)
endif
ifdef PROFILE
LDFLAGS+=-pg -g
endif
ifdef DEBUG
LDFLAGS+=-ggdb
else
LDFLAGS+=-s
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
