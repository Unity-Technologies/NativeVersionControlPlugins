include Makefile.srcs

CC = gcc
CXX = g++
AR = ar

GTK3_INCLUDE = -I/usr/include/gtk-3.0 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/pango-1.0 -I/usr/include/cairo -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/atk-1.0 -I/usr/include/harfbuzz
GTK3_LIBRARIES = -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0

CXXFLAGS += -O2 -g -fpermissive -Wno-deprecated-declarations $(GTK3_INCLUDE) $(P4PLUGIN_INCLUDE)
LDFLAGS += -g
LIBRARIES = -lstdc++ -lrt $(GTK3_LIBRARIES)

COMMON_MODULES = $(COMMON_SRCS:.c=.o)
COMMON_MODULES := $(COMMON_MODULES:.cpp=.o)

TESTSERVER_MODULES = $(TESTSERVER_SRCS:.c=.o)
TESTSERVER_MODULES := $(TESTSERVER_SRCS:.cpp=.o)
TESTSERVER_TARGET= Build/$(PLATFORM)/TestServer

P4PLUGIN_MODULES = $(P4PLUGIN_SRCS:.c=.o)
P4PLUGIN_MODULES := $(P4PLUGIN_MODULES:.cpp=.o)
P4PLUGIN_TARGET = PerforcePlugin
P4PLUGIN_LINK += $(LIBRARIES) -ldl -fPIC -no-pie

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
	$(CXX) $(CXXFLAGS) $(P4PLUGIN_INCLUDE) -D_LINUX -c $< -o $@

$(TESTSERVER_TARGET): $(COMMON_MODULES) $(TESTSERVER_MODULES)
	$(CXX) -g $(LDFLAGS) -o $@ $^

$(P4PLUGIN_TARGET): $(COMMON_MODULES) $(P4PLUGIN_MODULES)
	$(CXX) $(LDFLAGS) -o $@ $^  $(P4PLUGIN_LINK) -L./P4Plugin/Source/r19.1/lib/$(PLATFORM) 

clean:
	rm -f Build/*.* $(COMMON_MODULES) $(P4PLUGIN_MODULES) $(TESTSERVER_MODULES)
