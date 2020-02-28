s/include *<private\/\([a-zA-Z_.]*\)>/include "qmljs\/parser\/\1"/g
s/qtqmlcompilerglobal_p.h/qmljsglobal_p.h/g
s/<QtCore\/qglobal.h>/"qmljsglobal_p.h"/g
s/Q_QMLCOMPILER_PRIVATE_EXPORT/QML_PARSER_EXPORT/g
s/qqml/qml/g
s/QDECLARATIVE/QML/g
s/QQml/Qml/g
s/QQMLJS/QMLJS/g
s/Q_QML_EXPORT //g
s/Q_QML_PRIVATE_EXPORT/QML_PARSER_EXPORT/g
s/QT_BEGIN_NAMESPACE/QT_QML_BEGIN_NAMESPACE/g
s/QT_END_NAMESPACE/QT_QML_END_NAMESPACE/g

# adjust pri file
s/    \$\$PWD\/qmljsglobal_p.h/    $$PWD\/qmljsglobal_p.h \\\
    $$PWD\/qmljssourcelocation_p.h \\\
    $$PWD\/qmljsmemorypool_p.h \\\
    $$PWD\/qmldirparser_p.h \\\
    $$PWD\/qmljsgrammar_p.h \\\
    $$PWD\/qmljsparser_p.h/

s/    \$\$PWD\/qmljslexer.cpp/    $$PWD\/qmljslexer.cpp \\\
    $$PWD\/qmldirparser.cpp \\\
    $$PWD\/qmljsgrammar.cpp \\\
    $$PWD\/qmljsparser.cpp/
