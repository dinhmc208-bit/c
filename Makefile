CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -pthread -march=native
LDFLAGS = -pthread
# Uncomment to use OpenSSL for DES
# CXXFLAGS += -DUSE_OPENSSL
# LDFLAGS += -lssl -lcrypto

# Detect OS for epoll/kqueue
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    CXXFLAGS += -DUSE_EPOLL
endif
ifeq ($(UNAME_S),Darwin)
    CXXFLAGS += -DUSE_KQUEUE
endif
ifeq ($(UNAME_S),FreeBSD)
    CXXFLAGS += -DUSE_KQUEUE
endif

SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/nvnc

.PHONY: all clean directories

all: directories $(TARGET)

directories:
	@mkdir -p $(OBJDIR) $(BINDIR) output input bin

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete: $@"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(INCDIR) -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

