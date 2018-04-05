s/private\/qdeclarative/qml/g
s/qqml/qml/g
s/QDECLARATIVE/QML/g
s/QQml/Qml/g
s/QQMLJS/QMLJS/g
s/Q_QML_EXPORT //g
s/Q_QML_PRIVATE_EXPORT/QML_PARSER_EXPORT/

# adjust pri file
s/    \$\$PWD\/qmljsglobal_p.h/    $$PWD\/qmljsglobal_p.h \\\
    $$PWD\/qmldirparser_p.h \\\
    $$PWD\/qmlerror.h/
s/    \$\$PWD\/qmljsparser.cpp/    $$PWD\/qmljsparser.cpp \\\
    $$PWD\/qmldirparser.cpp \\\
    $$PWD\/qmlerror.cpp/
s/OTHER_FILES/DISTFILES/
