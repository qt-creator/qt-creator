TEMPLATE = subdirs

include(../../../qtcreator.pri)

qtHaveModule(declarative-private) {
    SUBDIRS += qmlpuppet
}
