TEMPLATE = subdirs # XXX: Avoid call the linker
TARGETPATH = Qt/labs/components/custom

INSTALL_IMPORTS = $$[QT_INSTALL_IMPORTS]

QML_FILES = \
        qmldir \
        BasicButton.qml \
        BusyIndicator.qml \
        ButtonBlock.qml \
        ButtonColumn.qml \
        ButtonRow.qml \
        ButtonGroup.js \
        Button.qml \
        CheckBox.qml \
        ChoiceList.qml \
        ProgressBar.qml \
        RadioButton.qml \
        ScrollDecorator.qml \
        ScrollIndicator.qml \
        Slider.qml \
        SpinBox.qml \
        Switch.qml \
        TextArea.qml \
        TextField.qml

QML_DIRS = \
        behaviors \
        private \
        styles \
        visuals

qmlfiles.files = $$QML_FILES
lessThan(QT_MAJOR_VERSION, 5):qmlfiles.sources = $$QML_FILES
qmlfiles.path = $$INSTALL_IMPORTS/$$TARGETPATH

qmldirs.files = $$QML_DIRS
lessThan(QT_MAJOR_VERSION, 5):qmldirs.sources = $$QML_DIRS
qmldirs.path = $$INSTALL_IMPORTS/$$TARGETPATH

INSTALLS += qmlfiles qmldirs
