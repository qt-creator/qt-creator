TEMPLATE = subdirs

include(../../../qtcreator.pri)

qtHaveModule(declarative-private) {
    QT += declarative-private core-private
    SUBDIRS += qmlpuppet
}
