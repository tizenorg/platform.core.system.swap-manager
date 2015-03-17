
debug_opt = -D__DEBUG

top_srcdir=.

TAGET=res

SRC_PARSER=\
		parser/icl_lexer.l++		\
		parser/icl_parser.y++		\

SRC_MAIN=\
		main.cpp						\
		CClient.cpp						\
		CConfig.cpp						\
		CFeatures.cpp					\
		CInterpreter.cpp				\
		probes/CAppListContainer.cpp	\
		probes/CLibListContainer.cpp	\
		probes/CProbe.cpp				\
		probes/CProbeFBI.cpp			\

SRC_PRECOMPILE=$(patsubst %.y++,%.cpp, $(patsubst %.l++,%.cpp, $(SRC_PARSER)))

ADD_RM=\
		parser/icl_parser.h

CPPSRC=\
		$(SRC_MAIN)				\
		$(SRC_PRECOMPILE)

LEX_OUTPUT_ROOT=lex.yy
LEX = flex
LEXCOMPILE = $(LEX) $(AM_LFLAGS) $(LFLAGS)

OBJ=$(patsubst %.cpp,%.o, $(CPPSRC))

CPP=g++
INCLUDE_CPPFLAGS= -I. -I./common -I./../daemon -I./probes -I./probes_data -I./instapi
CPP_LDFLAGS=-pthread

CPPFLAGS+=$(INCLUDE_CPPFLAGS)
CPPFLAGS+=$(debug_opt)
CPPFLAGS+=-std=c++11

YACC = bison -y
AM_YFLAGS = -d
YFLAGS = 
YLWRAP = $(top_srcdir)/ylwrap

YACCCOMPILE = $(YACC) $(AM_YFLAGS) $(YFLAGS)


%.cpp:%.l++
	$(am__skiplex) $(SHELL) $(YLWRAP) $< $(LEX_OUTPUT_ROOT).c $@ -- $(LEXCOMPILE)

%.cpp:%.y++
	$(am__skipyacc) $(SHELL) $(YLWRAP) $< y.tab.c $@ y.tab.h $*.h y.output $*.output -- $(YACCCOMPILE)

$(TAGET): $(SRC_PRECOMPILE) $(OBJ)
	$(CPP) $(CPP_LDFLAGS) $(OBJ) -o res

all: $(TAGET)
clean:
	rm $(OBJ) $(SRC_PRECOMPILE) $(ADD_RM)
