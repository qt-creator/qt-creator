TEMPLATE = subdirs

include(../../../qtcreator.pri)

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += declarative-private core-private
    SUBDIRS += qml2puppet
}

