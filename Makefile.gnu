include Makefile.srcs

CC = gcc
CXX = g++
AR = ar

CXXFLAGS += -O2 -g -fpermissive $(P4PLUGIN_INCLUDE)
LDFLAGS += -g
LIBRARIES = -lstdc++ -lrt

COMMON_MODULES = $(COMMON_SRCS:.c=.o)
COMMON_MODULES := $(COMMON_MODULES:.cpp=.o)

TESTSERVER_MODULES = $(TESTSERVER_SRCS:.c=.o)
TESTSERVER_MODULES := $(TESTSERVER_SRCS:.cpp=.o)
TESTSERVER_TARGET= Build/$(PLATFORM)/TestServer

P4PLUGIN_MODULES = $(P4PLUGIN_SRCS:.c=.o)
P4PLUGIN_MODULES := $(P4PLUGIN_MODULES:.cpp=.o)
P4PLUGIN_TARGET = PerforcePlugin
P4PLUGIN_LINK += $(LIBRARIES) -ldl

default: all

all: P4Plugin testserver

testserver: $(TESTSERVER_TARGET)
	@mkdir -p Build/$(PLATFORM)

P4Plugin: $(P4PLUGIN_TARGET)
	mkdir -p Build/$(PLATFORM)
	cp $(P4PLUGIN_TARGET) Build/$(PLATFORM)

Common: $(COMMON_MODULES)

Common/%.o : Common/%.cpp $(COMMON_INCLS)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

Test/Source/%.o : Test/Source/%.cpp $(TESTSERVER_INCLS)
	$(CXX) $(CXXFLAGS) $(TESTSERVER_INCLUDE) -c $< -o $@

P4Plugin/Source/%.o : P4Plugin/Source/%.cpp $(COMMON_INCLS) $(P4PLUGIN_INCLS)
	$(CXX) $(CXXFLAGS) $(P4PLUGIN_INCLUDE) -c $< -o $@

$(TESTSERVER_TARGET): $(COMMON_MODULES) $(TESTSERVER_MODULES)
	$(CXX) -g $(LDFLAGS) -o $@ $^

$(P4PLUGIN_TARGET): $(COMMON_MODULES) $(P4PLUGIN_MODULES)
	$(CXX) $(LDFLAGS) -o $@ $^  $(P4PLUGIN_LINK) -L./P4Plugin/Source/r16.1/lib/$(PLATFORM) 

clean:
	rm -f Build/*.* $(COMMON_MODULES) $(P4PLUGIN_MODULES) $(TESTSERVER_MODULES)
