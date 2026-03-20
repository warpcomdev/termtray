CXX ?= c++

QT_PACKAGE := $(shell if pkg-config --exists Qt6Widgets; then echo Qt6Widgets; elif pkg-config --exists Qt5Widgets; then echo Qt5Widgets; fi)

ifeq ($(QT_PACKAGE),)
$(error Install Qt development packages that provide pkg-config metadata for Qt6Widgets or Qt5Widgets)
endif

CXXFLAGS += -std=c++17 -O2 -Wall -Wextra -fPIC $(shell pkg-config --cflags $(QT_PACKAGE))
LDFLAGS += $(shell pkg-config --libs $(QT_PACKAGE))

.PHONY: all clean

all: termtray

termtray: main.o
	$(CXX) -o $@ $^ $(LDFLAGS)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f termtray main.o
