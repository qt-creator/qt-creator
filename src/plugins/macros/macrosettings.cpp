/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "macrosettings.h"

#include <QtCore/QSettings>

using namespace Macros::Internal;

static const char GROUP[] = "Macro";
static const char DEFAULT_DIRECTORY[] = "DefaultDirectory";
static const char SHOW_SAVE_DIALOG[] = "ShowSaveDialog";
static const char DIRECTORIES[] = "Directories";
static const char SHORTCUTIDS[] = "ShortcutIds";


MacroSettings::MacroSettings():
    showSaveDialog(false)
{
}

void MacroSettings::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(GROUP));
    s->setValue(QLatin1String(DEFAULT_DIRECTORY), defaultDirectory);
    s->setValue(QLatin1String(SHOW_SAVE_DIALOG), showSaveDialog);
    s->setValue(QLatin1String(DIRECTORIES), directories);
    s->setValue(QLatin1String(SHORTCUTIDS), shortcutIds);
    s->endGroup();
}

void MacroSettings::fromSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(GROUP));
    defaultDirectory = s->value(QLatin1String(DEFAULT_DIRECTORY), QString("")).toString();
    showSaveDialog = s->value(QLatin1String(SHOW_SAVE_DIALOG), false).toBool();
    directories = s->value(QLatin1String(DIRECTORIES)).toStringList();
    shortcutIds = s->value(QLatin1String(SHORTCUTIDS)).toMap();
    s->endGroup();
}

bool MacroSettings::equals(const MacroSettings &ms) const
{
    return defaultDirectory == ms.defaultDirectory &&
            shortcutIds == ms.shortcutIds &&
            directories == ms.directories &&
            showSaveDialog == ms.showSaveDialog;
}
