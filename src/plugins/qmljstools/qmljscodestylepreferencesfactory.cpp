// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscodestylepreferencesfactory.h"

#include "qmljscodestylesettings.h"
#include "qmljscodestylesettingspage.h"
#include "qmljseditor/qmljseditorconstants.h"
#include "qmljsindenter.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolstr.h"

#include <projectexplorer/project.h>
#include <texteditor/codestyleeditor.h>
#include <texteditor/indenter.h>
#include <utils/id.h>

#include <QString>
#include <QTextDocument>
#include <QWidget>

namespace QmlJSTools {

class QmlJsCodeStyleEditor final : public TextEditor::CodeStyleEditor
{
public:
    static QmlJsCodeStyleEditor *create(
        const TextEditor::ICodeStylePreferencesFactory *factory,
        ProjectExplorer::Project *project,
        TextEditor::ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr);

private:
    QmlJsCodeStyleEditor(QWidget *parent = nullptr);

    CodeStyleEditorWidget *createEditorWidget(
        const void * /*project*/,
        TextEditor::ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr) const override;
    QString previewText() const override;
    QString snippetProviderGroupId() const override;
};

QmlJsCodeStyleEditor *QmlJsCodeStyleEditor::create(
    const TextEditor::ICodeStylePreferencesFactory *factory,
    ProjectExplorer::Project *project,
    TextEditor::ICodeStylePreferences *codeStyle,
    QWidget *parent)
{
    auto editor = new QmlJsCodeStyleEditor{parent};
    editor->init(factory, wrapProject(project), codeStyle);
    return editor;
}

QmlJsCodeStyleEditor::QmlJsCodeStyleEditor(QWidget *parent)
    : CodeStyleEditor{parent}
{}

TextEditor::CodeStyleEditorWidget *QmlJsCodeStyleEditor::createEditorWidget(
    const void * /*project*/,
    TextEditor::ICodeStylePreferences *codeStyle,
    QWidget *parent) const
{
    auto qmlJSPreferences = dynamic_cast<QmlJSCodeStylePreferences *>(codeStyle);
    if (qmlJSPreferences == nullptr)
        return nullptr;
    auto widget = new Internal::QmlJSCodeStylePreferencesWidget(previewText(), parent);
    widget->setPreferences(qmlJSPreferences);
    return widget;
}

QString QmlJsCodeStyleEditor::previewText() const
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

QString QmlJsCodeStyleEditor::snippetProviderGroupId() const
{
    return QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID;
}

TextEditor::CodeStyleEditorWidget *QmlJSCodeStylePreferencesFactory::createCodeStyleEditor(
    const TextEditor::ProjectWrapper &project,
    TextEditor::ICodeStylePreferences *codeStyle,
    QWidget *parent) const
{
    return QmlJsCodeStyleEditor::create(
        this, ProjectExplorer::unwrapProject(project), codeStyle, parent);
}

Utils::Id QmlJSCodeStylePreferencesFactory::languageId()
{
    return Constants::QML_JS_SETTINGS_ID;
}

QString QmlJSCodeStylePreferencesFactory::displayName()
{
    return Tr::tr("Qt Quick");
}

TextEditor::ICodeStylePreferences *QmlJSCodeStylePreferencesFactory::createCodeStyle() const
{
    return new QmlJSCodeStylePreferences;
}

TextEditor::Indenter *QmlJSCodeStylePreferencesFactory::createIndenter(QTextDocument *doc) const
{
    return QmlJSEditor::createQmlJsIndenter(doc);
}

} // namespace QmlJSTools
