s/private\/qdeclarative/qml/g
s/qqml/qml/g
s/QDECLARATIVE/QML/g
s/QQml/Qml/g
s/QQMLJS/QMLJS/g
s/Q_QML_EXPORT //g

# adjust pri file
s/    \$\$PWD\/qmljsglobal_p.h/    $$PWD\/qmljsglobal_p.h \\\
    $$PWD\/qmldirparser_p.h \\\
    $$PWD\/qmlerror.h/
s/    \$\$PWD\/qmljsparser.cpp/    $$PWD\/qmljsparser.cpp \\\
    $$PWD\/qmldirparser.cpp \\\
    $$PWD\/qmlerror.cpp/
