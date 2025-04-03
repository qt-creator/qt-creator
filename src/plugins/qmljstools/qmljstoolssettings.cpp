// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscodestylesettings.h"

#include "qmljscodestylesettingspage.h"
#include "qmljseditor/qmljseditorconstants.h"
#include "qmljsindenter.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolssettings.h"
#include "qmljstoolstr.h"

#include <projectexplorer/project.h>

#include <texteditor/codestyleeditor.h>
#include <texteditor/codestylepool.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/indenter.h>
#include <texteditor/tabsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/id.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>

using namespace TextEditor;

namespace QmlJSTools {

const char idKey[] = "QmlJSGlobal";

static QmlJSCodeStylePreferences *m_globalCodeStyle = nullptr;

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
        static const QString defaultPreviewText = R"(import QtQuick 1.0

Rectangle {
    width: 360
    height: 360
    Text {
        anchors.centerIn: parent
        text: "Hello World"
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            Qt.quit();
        }
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

QmlJSToolsSettings::QmlJSToolsSettings()
{
    QTC_ASSERT(!m_globalCodeStyle, return);

    // code style factory
    ICodeStylePreferencesFactory *factory =  new QmlJSCodeStylePreferencesFactory;

    TextEditorSettings::registerCodeStyleFactory(factory);

    // code style pool
    auto pool = new CodeStylePool(factory, this);
    TextEditorSettings::registerCodeStylePool(Constants::QML_JS_SETTINGS_ID, pool);

    // global code style settings
    m_globalCodeStyle = new QmlJSCodeStylePreferences(this);
    m_globalCodeStyle->setDelegatingPool(pool);
    m_globalCodeStyle->setDisplayName(Tr::tr("Global", "Settings"));
    m_globalCodeStyle->setId(idKey);
    pool->addCodeStyle(m_globalCodeStyle);
    TextEditorSettings::registerCodeStyle(QmlJSTools::Constants::QML_JS_SETTINGS_ID, m_globalCodeStyle);

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
    QmlJSCodeStyleSettings qtQmlJSSetings;
    qtQmlJSSetings.lineLength = 80;
    qtCodeStyle->setCodeStyleSettings(qtQmlJSSetings);
    pool->addCodeStyle(qtCodeStyle);

    // default delegate for global preferences
    m_globalCodeStyle->setCurrentDelegate(qtCodeStyle);

    pool->loadCustomCodeStyles();

    // load global settings (after built-in settings are added to the pool)
    m_globalCodeStyle->fromSettings(QmlJSTools::Constants::QML_JS_SETTINGS_ID);

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

    delete m_globalCodeStyle;
    m_globalCodeStyle = nullptr;
}

QmlJSCodeStylePreferences *QmlJSToolsSettings::globalCodeStyle()
{
    return m_globalCodeStyle;
}

} // namespace QmlJSTools
