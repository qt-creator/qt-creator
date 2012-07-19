/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "cdboptions.h"

#include <QSettings>

static const char settingsGroupC[] = "CDB2";
static const char symbolPathsKeyC[] = "SymbolPaths";
static const char sourcePathsKeyC[] = "SourcePaths";
static const char breakEventKeyC[] = "BreakEvent";
static const char additionalArgumentsKeyC[] = "AdditionalArguments";
static const char cdbConsoleKeyC[] = "CDB_Console";
static const char breakpointCorrectionKeyC[] = "BreakpointCorrection";

namespace Debugger {
namespace Internal {

CdbOptions::CdbOptions() : cdbConsole(false), breakpointCorrection(true)
{
}

QString CdbOptions::settingsGroup()
{
    return QLatin1String(settingsGroupC);
}

void CdbOptions::clear()
{
    symbolPaths.clear();
    sourcePaths.clear();
    cdbConsole = false;
}

QStringList CdbOptions::oldEngineSymbolPaths(const QSettings *s)
{
    return s->value(QLatin1String("CDB/SymbolPaths")).toStringList();
}

void CdbOptions::fromSettings(QSettings *s)
{
    clear();
    const QString keyRoot = QLatin1String(settingsGroupC) + QLatin1Char('/');
    additionalArguments = s->value(keyRoot + QLatin1String(additionalArgumentsKeyC), QString()).toString();
    symbolPaths = s->value(keyRoot + QLatin1String(symbolPathsKeyC), QStringList()).toStringList();
    sourcePaths = s->value(keyRoot + QLatin1String(sourcePathsKeyC), QStringList()).toStringList();
    breakEvents = s->value(keyRoot + QLatin1String(breakEventKeyC), QStringList()).toStringList();
    cdbConsole = s->value(keyRoot + QLatin1String(cdbConsoleKeyC), QVariant(false)).toBool();
    breakpointCorrection = s->value(keyRoot + QLatin1String(breakpointCorrectionKeyC), QVariant(true)).toBool();
}

void CdbOptions::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(symbolPathsKeyC), symbolPaths);
    s->setValue(QLatin1String(sourcePathsKeyC), sourcePaths);
    s->setValue(QLatin1String(breakEventKeyC), breakEvents);
    s->setValue(QLatin1String(additionalArgumentsKeyC), additionalArguments);
    s->setValue(QLatin1String(cdbConsoleKeyC), QVariant(cdbConsole));
    s->setValue(QLatin1String(breakpointCorrectionKeyC), QVariant(breakpointCorrection));
    s->endGroup();
}

bool CdbOptions::equals(const CdbOptions &rhs) const
{
    return cdbConsole == rhs.cdbConsole
            && breakpointCorrection == rhs.breakpointCorrection
            && additionalArguments == rhs.additionalArguments
            && symbolPaths == rhs.symbolPaths
            && sourcePaths == rhs.sourcePaths
            && breakEvents == rhs.breakEvents;
}

} // namespace Internal
} // namespace Debugger
