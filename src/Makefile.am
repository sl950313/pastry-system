AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = pastry
pastry_SOURCES = main.cpp leafset.cpp link.cpp node.cpp route-table.cpp set.cpp sha1.cpp test-id.cpp  test-link.cpp
#pastry_LDFLAGS = -lpthread -lglog -lgtest
pastry_CPPFLAGS = -std=c++11
noinst_HEADERS = defines.h  id.h  leafset.h  link.h  route-table.h  set.h
include_HEADERS = node.h
LIBS = -lpthread -lglog -lgtest
