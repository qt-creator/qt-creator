s/qdeclarative/qml/g
s/QDECLARATIVE/QML/g
s/QDeclarative/Qml/g

# adjust pri file
s/    \$\$PWD\/qmljsglobal_p.h/    $$PWD\/qmljsglobal_p.h \\\
    $$PWD\/qmldirparser_p.h \\\
    $$PWD\/qmlerror.h/
s/    \$\$PWD\/qmljsparser.cpp/    $$PWD\/qmljsparser.cpp \\\
    $$PWD\/qmldirparser.cpp \\\
    $$PWD\/qmlerror.cpp/
