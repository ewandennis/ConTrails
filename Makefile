INCDIR=-I/usr/local/include/cairo -I/usr/local/include/libmongoc-1.0/ -I/usr/local/include/libbson-1.0/
LIBDIR=-L /usr/local/lib/
LIBS=-lcairo -lmongoc-1.0 -lbson-1.0
CXXFLAGS=-std=c++11 $(INCDIR) -O3
LDFLAGS=$(LIBS) $(LIBDIR)

OBJDIR=obj
OBJECTS=$(addprefix $(OBJDIR)/, db.o flighttracks.o colour.o main.o)
BINARY=render

all: $(BINARY) $(OBJECTS)

$(BINARY): $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	-rm $(OBJECTS) $(BINARY)

