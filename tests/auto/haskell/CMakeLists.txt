add_qtc_test(tst_tokenizer
  DEPENDS Qt::Core Qt::Test
  INCLUDES ../../../src/plugins/haskell
  SOURCES
    tst_tokenizer.cpp
    ../../../src/plugins/haskell/haskelltokenizer.cpp
    ../../../src/plugins/haskell/haskelltokenizer.h
)
