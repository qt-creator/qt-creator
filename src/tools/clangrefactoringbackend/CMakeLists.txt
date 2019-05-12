add_qtc_executable(clangrefactoringbackend
  DEPENDS clangbackendipc
  INCLUDES source
  SOURCES
    clangrefactoringbackendmain.cpp
    source/clangquery.cpp source/clangquery.h
    source/clangrefactoringbackend_global.h
    source/clangtool.cpp source/clangtool.h
    source/findcursorusr.h
    source/findlocationsofusrs.h
    source/findusrforcursoraction.cpp source/findusrforcursoraction.h
    source/macropreprocessorcallbacks.cpp source/macropreprocessorcallbacks.h
    source/refactoringcompilationdatabase.cpp source/refactoringcompilationdatabase.h
    source/refactoringserver.cpp source/refactoringserver.h
    source/sourcelocationsutils.h
    source/symbolfinder.cpp source/symbolfinder.h
    source/symbollocationfinderaction.cpp source/symbollocationfinderaction.h
)
