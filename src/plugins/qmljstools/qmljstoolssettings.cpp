// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscodestylesettings.h"

#include "qmljsindenter.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolssettings.h"
#include "qmlformatsettings.h"
#include "qmljstoolstr.h"

#include <projectexplorer/project.h>

#include <qmljseditor/qmljseditorconstants.h>

#include <texteditor/codestyleeditor.h>
#include <texteditor/codestylepool.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/indenter.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/shutdownguard.h>

using namespace TextEditor;
using namespace Utils;

namespace QmlJSTools {

const char idKey[] = "QmlJSGlobal";

class QmlJsCodeStyleEditor final : public CodeStyleEditor
{
public:
    static QmlJsCodeStyleEditor *create(
        const ICodeStylePreferencesFactory *factory,
        ProjectExplorer::Project *project,
        ICodeStylePreferences *codeStyle,
        QWidget *parent)
    {
        auto editor = new QmlJsCodeStyleEditor{parent};
        editor->init(factory, wrapProject(project), codeStyle);
        return editor;
    }

private:
    QmlJsCodeStyleEditor(QWidget *parent)
        : CodeStyleEditor{parent}
    {}

    CodeStyleEditorWidget *createEditorWidget(
        const void * /*project*/,
        ICodeStylePreferences *codeStyle,
        QWidget *parent) const final
    {
        auto qmlJSPreferences = dynamic_cast<QmlJSCodeStylePreferences *>(codeStyle);
        if (qmlJSPreferences == nullptr)
            return nullptr;
        auto widget = new Internal::QmlJSCodeStylePreferencesWidget(previewText(), parent);
        widget->setPreferences(qmlJSPreferences);
        return widget;
    }

    QString previewText() const final
    {
        static const QString defaultPreviewText = R"(import QtQuick
import QtQuick.Templates
import QtQuick.Controls
Rectangle {
    // Object
    MouseArea {
        onPressed: {
            console.log("pressed");;;
        }
        anchors.fill: parent
    }

    id: myId

    // Binding
    height: myHeight

    // Function
    function computeValue(x, y) {
        return x + y;;
    }

    // Property (scattered)
    property int z: 3

    // Signal
    signal triggered

    // Another object
    Item {
        id: childItem
        width: 50
        height: 50
    }

    // Required property
    required property int requiredValue

    // Alias
    property alias childRef: childItem

    // Inline binding with JS
    width: computeValue(100, 200)

    // Enum
    enum Mode {
        ModeA,
        ModeB,
        ModeC
    }

    // Another function
    function f() {
        ;;
    }

    // Attached property
    Component.onCompleted: {
        console.log("Completed");;;
    }

    // Readonly property
    readonly property int constValue: 42

    // Another signal
    signal valueChanged(int newValue)

    // Grouped property (font)
    property font myFont: Qt.font({
        family: "Arial",
        pixelSize: 14
    })

    // Another object (empty)
    QtObject {
    }

    // Binding using ternary
    opacity: enabled ? 1.0 : 0.5

    // More properties (intentionally late)
    property bool enabled: true
    property string name: "qmlformat"

    // Function using arrow syntax
    function iterate() {
        [1,2,3].forEach(x => console.log(x));;;
    }

    // Signal handler style
    onTriggered: {
        console.log("triggered handler")
    }

    // Another attached property
    Component.onDestruction: {
        console.log("Destroyed")
    }

    // Nested object with mixed content
    Text {
        text: name
        anchors.centerIn: parent

        Component.onCompleted: {
            console.log("Text ready");;
        }
    }

    // More bindings
    y: 20
    x: 10

    // Another enum (tests grouping behavior)
    enum Status {
        Idle,
        Running,
        Stopped
    }

    // Complex JS binding
    property var data: ({
        a: 1,
        b: 2,
        c: [1,2,3]
    })

    // Another function after everything
    function zFunc() {
        let tmp = 0;;;
        for (let i = 0; i < 5; ++i) {
            tmp += i;
        }
        return tmp;
    }
})";

        return defaultPreviewText;
    }

    QString snippetProviderGroupId() const final
    {
        return QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID;
    }
};


// QmlJSCodeStylePreferencesFactory

class QmlJSCodeStylePreferencesFactory final : public ICodeStylePreferencesFactory
{
public:
    QmlJSCodeStylePreferencesFactory() = default;

private:
    CodeStyleEditorWidget *createCodeStyleEditor(
            const ProjectWrapper &project,
            ICodeStylePreferences *codeStyle,
            QWidget *parent) const final
    {
        return QmlJsCodeStyleEditor::create(
                    this, ProjectExplorer::unwrapProject(project), codeStyle, parent);
    }

    Utils::Id languageId() final
    {
        return Constants::QML_JS_SETTINGS_ID;
    }

    QString displayName() final
    {
        return Tr::tr("Qt Quick");
    }

    ICodeStylePreferences *createCodeStyle() const final
    {
        return new QmlJSCodeStylePreferences;
    }

    Indenter *createIndenter(QTextDocument *doc) const final
    {
        return QmlJSEditor::createQmlJsIndenter(doc);
    }
};

// QmlJSToolsSettings

class QmlJSToolsSettings final : public QObject
{
public:
    QmlJSToolsSettings();
    ~QmlJSToolsSettings() final;

    QmlJSCodeStylePreferences m_globalCodeStyle;
};

QmlJSToolsSettings::QmlJSToolsSettings()
{
    // code style factory
    ICodeStylePreferencesFactory *factory =  new QmlJSCodeStylePreferencesFactory;

    TextEditorSettings::registerCodeStyleFactory(factory);

    // code style pool
    auto pool = new CodeStylePool(factory, this);
    TextEditorSettings::registerCodeStylePool(Constants::QML_JS_SETTINGS_ID, pool);

    // global code style settings
    m_globalCodeStyle.setDelegatingPool(pool);
    m_globalCodeStyle.setDisplayName(Tr::tr("Global", "Settings"));
    m_globalCodeStyle.setId(idKey);
    pool->addCodeStyle(&m_globalCodeStyle);
    TextEditorSettings::registerCodeStyle(QmlJSTools::Constants::QML_JS_SETTINGS_ID, &m_globalCodeStyle);

    // built-in settings
    // Qt style
    auto qtCodeStyle = new QmlJSCodeStylePreferences;
    qtCodeStyle->setId("qt");
    qtCodeStyle->setDisplayName(Tr::tr("Qt"));
    qtCodeStyle->setReadOnly(true);
    TabSettings qtTabSettings;
    qtTabSettings.m_tabPolicy = TabSettings::SpacesOnlyTabPolicy;
    qtTabSettings.m_tabSize = 4;
    qtTabSettings.m_indentSize = 4;
    qtTabSettings.m_continuationAlignBehavior = TabSettings::ContinuationAlignWithIndent;
    qtCodeStyle->setTabSettings(qtTabSettings);

    connect(&QmlFormatSettings::instance(), &QmlFormatSettings::qmlformatIniCreated,
            this, [](Utils::FilePath qmlformatIniPath) {
        QmlJSCodeStyleSettings s;
        s.lineLength = 80;
        const Utils::Result<QByteArray> fileContents = qmlformatIniPath.fileContents();
        if (fileContents)
            s.qmlformatIniContent = QString::fromUtf8(*fileContents);
        auto csPool = TextEditorSettings::codeStylePool(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
        QTC_ASSERT(csPool, return);
        auto builtInCodeStyles = csPool->builtInCodeStyles();
        for (auto codeStyle : builtInCodeStyles) {
            if (auto qtCodeStyle = dynamic_cast<QmlJSCodeStylePreferences *>(codeStyle))
                qtCodeStyle->setCodeStyleSettings(s);
        }
    });

    pool->addCodeStyle(qtCodeStyle);

    // default delegate for global preferences
    m_globalCodeStyle.setCurrentDelegate(qtCodeStyle);

    pool->loadCustomCodeStyles();

    // load global settings (after built-in settings are added to the pool)
    m_globalCodeStyle.fromSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID);

    // mimetypes to be handled
    using namespace Utils::Constants;
    TextEditorSettings::registerMimeTypeForLanguageId(QML_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(QMLUI_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(QBS_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(QMLPROJECT_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(QMLTYPES_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(JS_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::registerMimeTypeForLanguageId(JSON_MIMETYPE, Constants::QML_JS_SETTINGS_ID);
}

QmlJSToolsSettings::~QmlJSToolsSettings()
{
    TextEditorSettings::unregisterCodeStyle(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::unregisterCodeStylePool(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
    TextEditorSettings::unregisterCodeStyleFactory(QmlJSTools::Constants::QML_JS_SETTINGS_ID);
}

static QmlJSToolsSettings &toolsSettings()
{
    static GuardedObject<QmlJSToolsSettings> theQmlJSToolsSettings;
    return theQmlJSToolsSettings;
}

QmlJSCodeStylePreferences *globalQmlJSCodeStyle()
{
    return &toolsSettings().m_globalCodeStyle;
}

void Internal::setupQmlJSToolsSettings()
{
    (void) toolsSettings();
}

} // namespace QmlJSTools
