/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmljscodestylepreferencesfactory.h"
#include "qmljscodestylesettingspage.h"
#include "qmljstoolsconstants.h"
#include "qmljsindenter.h"
#include <texteditor/simplecodestylepreferences.h>
#include <texteditor/tabsettings.h>
#include <texteditor/snippets/isnippetprovider.h>
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

QString QmlJSCodeStylePreferencesFactory::languageId()
{
    return Constants::QML_JS_SETTINGS_ID;
}

QString QmlJSCodeStylePreferencesFactory::displayName()
{
    return Constants::QML_JS_SETTINGS_NAME;
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

TextEditor::ISnippetProvider *QmlJSCodeStylePreferencesFactory::snippetProvider() const
{
    const QList<TextEditor::ISnippetProvider *> &providers =
    ExtensionSystem::PluginManager::getObjects<TextEditor::ISnippetProvider>();
    foreach (TextEditor::ISnippetProvider *provider, providers)
        if (provider->groupId() == QLatin1String(QmlJSEditor::Constants::QML_SNIPPETS_GROUP_ID))
            return provider;
    return 0;
}

QString QmlJSCodeStylePreferencesFactory::previewText() const
{
    return QLatin1String(defaultPreviewText);
}

