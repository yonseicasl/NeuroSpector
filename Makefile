# Directories
SRCDIR=src
OBJDIR=obj
SUBDIRS=$(dir $(wildcard $(SRCDIR)/*/.))

# Flags
CXX=g++
SUBFLAGS=$(addprefix -I, $(patsubst %/, %, $(SUBDIRS)))
CXXFLAGS=-g -Wall -O3 -std=c++11 -DCSV -DSIMPLE -I$(SRCDIR) $(SUBFLAGS) 
#CXXFLAGS=-g -Wall -O3 -std=c++11 -I$(SRCDIR) $(SUBFLAGS) 
LDFLAGS=
LIBFLAGS=-lpthread

# Sources(/src) 
SRCS=$(wildcard $(SRCDIR)/*.cc)
HDRS=$(wildcard $(SRCDIR)/*.h) 
OBJS=$(SRCS:$(SRCDIR)/%.cc=$(OBJDIR)/%.o) 
# Sources(/src/*) 
SUBSRCS=$(wildcard $(SRCDIR)/*/*.cc)
SUBHDRS=$(wildcard $(SRCDIR)/*/*.h)
SUBOBJS=$(addprefix $(OBJDIR)/, $(notdir $(patsubst %.cc, %.o, $(SUBSRCS))))
# Executable
EXE=neurospector

# Targets
.PHONY: default
default: $(EXE)

$(EXE): $(SUBOBJS) $(OBJS) 
	@echo "# Makefile Target: $@" 
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^ $(LIBFLAGS) 

$(OBJDIR)/%.o: $(SRCDIR)/%.cc $(HDRS)
	@mkdir -pv $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $@ -c $< 

$(OBJDIR)/%.o: $(SRCDIR)/*/%.cc $(SUBHDRS)
	@mkdir -pv $(OBJDIR)
	$(CXX) $(CXXFLAGS) -o $@ -c $< 

.PHONY: clean
clean:
	@rm -f $(OBJS) $(SUBOBJS) $(EXE)
	@rm -rf $(OBJDIR)
	@echo "# Makefile Clean: $(OBJDIR)/'s [ $(notdir $(OBJS) $(SUBOBJS) ] and [ $(EXE)) ] are removed" 
