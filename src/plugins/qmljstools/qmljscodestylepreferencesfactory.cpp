// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljscodestylepreferencesfactory.h"

#include "qmljscodestylepreferences.h"
#include "qmljscodestylesettingspage.h"
#include "qmljsindenter.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolstr.h"

#include <qmljseditor/qmljseditorconstants.h>

using namespace QmlJSTools;

static const char *defaultPreviewText =
    "import QtQuick 1.0\n"
    "\n"
    "Rectangle {\n"
    "    width: 360\n"
    "    height: 360\n"
    "    Text {\n"
    "        anchors.centerIn: parent\n"
    "        text: \"Hello World\"\n"
    "    }\n"
    "    MouseArea {\n"
    "        anchors.fill: parent\n"
    "        onClicked: {\n"
    "            Qt.quit();\n"
    "        }\n"
    "    }\n"
    "}\n";

QmlJSCodeStylePreferencesFactory::QmlJSCodeStylePreferencesFactory() = default;

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
    return new QmlJSCodeStylePreferences();
}

TextEditor::CodeStyleEditorWidget *QmlJSCodeStylePreferencesFactory::createEditor(
    TextEditor::ICodeStylePreferences *preferences,
    ProjectExplorer::Project *project,
    QWidget *parent) const
{
    Q_UNUSED(project)
    auto qmlJSPreferences = qobject_cast<QmlJSCodeStylePreferences *>(preferences);
    if (!qmlJSPreferences)
        return nullptr;
    auto widget = new Internal::QmlJSCodeStylePreferencesWidget(this, parent);
    widget->setPreferences(qmlJSPreferences);
    return widget;
}

TextEditor::Indenter *QmlJSCodeStylePreferencesFactory::createIndenter(QTextDocument *doc) const
{
    return new QmlJSEditor::Internal::Indenter(doc);
}

QString QmlJSCodeStylePreferencesFactory::snippetProviderGroupId() const
{
    return QString(QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID);
}

QString QmlJSCodeStylePreferencesFactory::previewText() const
{
    return QLatin1String(defaultPreviewText);
}

