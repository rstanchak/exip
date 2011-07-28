# GrammarGen module build

GRAMMAR_GEN_OBJ = $(GRAMMAR_GEN_BIN)/genUtils.o $(GRAMMAR_GEN_BIN)/grammarGenerator.o $(GRAMMAR_GEN_BIN)/protoGrammars.o

$(GRAMMAR_GEN_BIN)/genUtils.o : $(GRAMMAR_GEN_SRC)/include/genUtils.h $(GRAMMAR_GEN_SRC)/src/genUtils.c
		$(CC) -c $(CFLAGS) $(GRAMMAR_GEN_SRC)/src/genUtils.c -o $(GRAMMAR_GEN_BIN)/genUtils.o

$(GRAMMAR_GEN_BIN)/grammarGenerator.o : $(PUBLIC_INCLUDE_DIR)/grammarGenerator.h $(GRAMMAR_GEN_SRC)/src/grammarGenerator.c $(PUBLIC_INCLUDE_DIR)/schema.h
		$(CC) -c $(CFLAGS) $(GRAMMAR_GEN_SRC)/src/grammarGenerator.c -o $(GRAMMAR_GEN_BIN)/grammarGenerator.o

$(GRAMMAR_GEN_BIN)/protoGrammars.o : $(GRAMMAR_GEN_SRC)/include/protoGrammars.h $(GRAMMAR_GEN_SRC)/src/protoGrammars.c
		$(CC) -c $(CFLAGS) $(GRAMMAR_GEN_SRC)/src/protoGrammars.c -o $(GRAMMAR_GEN_BIN)/protoGrammars.o