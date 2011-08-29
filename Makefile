include makeinclude

OBJ	= main.o hash.o storage.o queryhandler.o search.o

all: mulletdb

mulletdb: $(OBJ)
	$(LD) $(LDFLAGS) -o mulletdb $(OBJ) $(LIBS)

clean:
	rm -f *.o
	rm -f mulletdb

allclean: clean
	rm -f makeinclude configure.paths platform.h
	
install: all
	./makeinstall

makeinclude:
	@echo please run ./configure
	@false

SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) -g -O0 $(CXXFLAGS) $(INCLUDES) -c $<
