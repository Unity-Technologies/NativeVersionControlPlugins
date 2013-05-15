include Makefile.srcs

CC = gcc
CXX = g++
AR = ar

# CXXFLAGS = -O3 -arch i386 -g
CXXFLAGS += -g -fpermissive $(P4PLUGIN_INCLUDE)
LIBRARIES = -lstdc++ -lrt

COMMON_MODULES = $(COMMON_SRCS:.c=.o)
COMMON_MODULES := $(COMMON_MODULES:.cpp=.o)

P4PLUGIN_MODULES = $(P4PLUGIN_SRCS:.c=.o)
P4PLUGIN_MODULES := $(P4PLUGIN_MODULES:.cpp=.o)
P4PLUGIN_TARGET = PerforcePlugin
P4PLUGIN_LINK += $(LIBRARIES) -ldl

SVNPLUGIN_MODULES = $(SVNPLUGIN_SRCS:.c=.o)
SVNPLUGIN_MODULES := $(SVNPLUGIN_MODULES:.cpp=.o)
SVNPLUGIN_TARGET = SubversionPlugin
SVNPLUGIN_LINK += $(LIBRARIES)

default: all

all: P4Plugin SvnPlugin

P4Plugin: $(P4PLUGIN_TARGET)
	mkdir -p Build/$(PLATFORM)
	cp $(P4PLUGIN_TARGET) Build/$(PLATFORM)

SvnPlugin: $(SVNPLUGIN_TARGET)
	mkdir -p Build/$(PLATFORM)
	cp $(SVNPLUGIN_TARGET) Build/$(PLATFORM)

Common: $(COMMON_MODULES)

Common/%.o : Common/%.cpp $(COMMON_INCLS)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

P4Plugin/Source/%.o : P4Plugin/Source/%.cpp $(COMMON_INCLS) $(P4PLUGIN_INCLS)
	$(CXX) $(CXXFLAGS) $(P4PLUGIN_INCLUDE) -c $< -o $@

SvnPlugin/Source/%.o : SvnPlugin/Source/%.cpp $(COMMON_INCLS) $(SVNPLUGIN_INCLS)
	$(CXX) $(CXXFLAGS) $(SVNPLUGIN_INCLUDE) -c $< -o $@

$(P4PLUGIN_TARGET): $(COMMON_MODULES) $(P4PLUGIN_MODULES)
	$(CXX) $(LDFLAGS) -o $@ $^  $(P4PLUGIN_LINK) ./P4Plugin/Source/r12.2/lib/$(PLATFORM)/libssl.a ./P4Plugin/Source/r12.2/lib/$(PLATFORM)/libcrypto.a -L./P4Plugin/Source/r12.2/lib/$(PLATFORM)

$(SVNPLUGIN_TARGET): $(COMMON_MODULES) $(SVNPLUGIN_MODULES)
	$(CXX) $(LDFLAGS) -o $@ $^ $(SVNPLUGIN_LINK)

clean:
	rm -f Build/*.* $(COMMON_MODULES) $(P4PLUGIN_MODULES) $(SVNPLUGIN_MODULES)
