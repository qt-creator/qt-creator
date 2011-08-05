/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmljscodestylesettingsfactory.h"
#include "qmljscodestylesettingspage.h"
#include "qmljstoolsconstants.h"
#include "qmljsindenter.h"
#include <texteditor/tabpreferences.h>
#include <texteditor/tabsettings.h>
#include <QtGui/QLayout>

using namespace QmlJSTools;

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

TextEditor::IFallbackPreferences *QmlJSCodeStylePreferencesFactory::createPreferences(
    const QList<TextEditor::IFallbackPreferences *> &fallbacks) const
{
    Q_UNUSED(fallbacks);
    return 0;
}

QWidget *QmlJSCodeStylePreferencesFactory::createEditor(TextEditor::IFallbackPreferences *preferences,
                                                           TextEditor::TabPreferences *tabPreferences,
                                                           QWidget *parent) const
{
    Q_UNUSED(preferences)

    Internal::QmlJSCodeStylePreferencesWidget *widget = new Internal::QmlJSCodeStylePreferencesWidget(parent);
    widget->layout()->setMargin(0);
    widget->setTabPreferences(tabPreferences);
    return widget;
}

TextEditor::Indenter *QmlJSCodeStylePreferencesFactory::createIndenter() const
{
    return new QmlJSEditor::Internal::Indenter();
}

