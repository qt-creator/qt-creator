/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmljscodestylepreferencesfactory.h"
#include "qmljscodestylesettingspage.h"
#include "qmljstoolsconstants.h"
#include "qmljsindenter.h"
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/snippets/snippetprovider.h>
#include <extensionsystem/pluginmanager.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <QLayout>

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

QmlJSCodeStylePreferencesFactory::QmlJSCodeStylePreferencesFactory()
{
}

Core::Id QmlJSCodeStylePreferencesFactory::languageId()
{
    return Constants::QML_JS_SETTINGS_ID;
}

QString QmlJSCodeStylePreferencesFactory::displayName()
{
    return QLatin1String(Constants::QML_JS_SETTINGS_NAME);
}

TextEditor::ICodeStylePreferences *QmlJSCodeStylePreferencesFactory::createCodeStyle() const
{
    return new TextEditor::SimpleCodeStylePreferences();
}

QWidget *QmlJSCodeStylePreferencesFactory::createEditor(TextEditor::ICodeStylePreferences *preferences,
                                                           QWidget *parent) const
{
    Internal::QmlJSCodeStylePreferencesWidget *widget = new Internal::QmlJSCodeStylePreferencesWidget(parent);
    widget->layout()->setMargin(0);
    widget->setPreferences(preferences);
    return widget;
}

TextEditor::Indenter *QmlJSCodeStylePreferencesFactory::createIndenter() const
{
    return new QmlJSEditor::Internal::Indenter();
}

TextEditor::SnippetProvider *QmlJSCodeStylePreferencesFactory::snippetProvider() const
{
    return ExtensionSystem::PluginManager::getObject<TextEditor::SnippetProvider>(
        [](TextEditor::SnippetProvider *provider) {
            return provider->groupId() == QLatin1String(QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID);
        });
}

QString QmlJSCodeStylePreferencesFactory::previewText() const
{
    return QLatin1String(defaultPreviewText);
}

