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

#include "cppcodestylesettingsfactory.h"
#include "cppcodestylesettings.h"
#include "cppcodestylesettingspage.h"
#include "cppcodestylepreferences.h"
#include "cpptoolsconstants.h"
#include <texteditor/tabpreferences.h>
#include <texteditor/tabsettings.h>
#include <QtGui/QLayout>

using namespace CppTools;

CppCodeStylePreferencesFactory::CppCodeStylePreferencesFactory()
{
}

QString CppCodeStylePreferencesFactory::languageId()
{
    return Constants::CPP_SETTINGS_ID;
}

QString CppCodeStylePreferencesFactory::displayName()
{
    return Constants::CPP_SETTINGS_NAME;
}

TextEditor::IFallbackPreferences *CppCodeStylePreferencesFactory::createPreferences(
    const QList<TextEditor::IFallbackPreferences *> &fallbacks) const
{
    return new CppCodeStylePreferences(fallbacks);
}

QWidget *CppCodeStylePreferencesFactory::createEditor(TextEditor::IFallbackPreferences *preferences,
                                                           TextEditor::TabPreferences *tabPreferences,
                                                           QWidget *parent) const
{
    CppCodeStylePreferences *cppPreferences = qobject_cast<CppCodeStylePreferences *>(preferences);
    if (!cppPreferences)
        return 0;
    Internal::CppCodeStylePreferencesWidget *widget = new Internal::CppCodeStylePreferencesWidget(parent);
    widget->layout()->setMargin(0);
    widget->setPreferences(cppPreferences, tabPreferences);
    return widget;
}

