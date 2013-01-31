/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cdboptions.h"

#include <QSettings>

static const char settingsGroupC[] = "CDB2";
static const char symbolPathsKeyC[] = "SymbolPaths";
static const char sourcePathsKeyC[] = "SourcePaths";
static const char breakEventKeyC[] = "BreakEvent";
static const char breakFunctionsKeyC[] = "BreakFunctions";
static const char additionalArgumentsKeyC[] = "AdditionalArguments";
static const char cdbConsoleKeyC[] = "CDB_Console";
static const char breakpointCorrectionKeyC[] = "BreakpointCorrection";
static const char ignoreFirstChanceAccessViolationKeyC[] = "IgnoreFirstChanceAccessViolation";

namespace Debugger {
namespace Internal {

const char *CdbOptions::crtDbgReport = "CrtDbgReport";

CdbOptions::CdbOptions()
    : cdbConsole(false)
    , breakpointCorrection(true)
    , ignoreFirstChanceAccessViolation(false)
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
    breakpointCorrection = true;
    cdbConsole = ignoreFirstChanceAccessViolation = false;
    breakEvents.clear();
    breakFunctions.clear();
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
    breakFunctions = s->value(keyRoot + QLatin1String(breakFunctionsKeyC), QStringList()).toStringList();
    cdbConsole = s->value(keyRoot + QLatin1String(cdbConsoleKeyC), QVariant(false)).toBool();
    breakpointCorrection = s->value(keyRoot + QLatin1String(breakpointCorrectionKeyC), QVariant(true)).toBool();
    ignoreFirstChanceAccessViolation = s->value(keyRoot + QLatin1String(ignoreFirstChanceAccessViolationKeyC), false).toBool();
}

void CdbOptions::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(symbolPathsKeyC), symbolPaths);
    s->setValue(QLatin1String(sourcePathsKeyC), sourcePaths);
    s->setValue(QLatin1String(breakEventKeyC), breakEvents);
    s->setValue(QLatin1String(breakFunctionsKeyC), breakFunctions);
    s->setValue(QLatin1String(additionalArgumentsKeyC), additionalArguments);
    s->setValue(QLatin1String(cdbConsoleKeyC), QVariant(cdbConsole));
    s->setValue(QLatin1String(breakpointCorrectionKeyC), QVariant(breakpointCorrection));
    s->setValue(QLatin1String(ignoreFirstChanceAccessViolationKeyC), QVariant(ignoreFirstChanceAccessViolation));
    s->endGroup();
}

bool CdbOptions::equals(const CdbOptions &rhs) const
{
    return cdbConsole == rhs.cdbConsole
            && breakpointCorrection == rhs.breakpointCorrection
            && ignoreFirstChanceAccessViolation == rhs.ignoreFirstChanceAccessViolation
            && additionalArguments == rhs.additionalArguments
            && symbolPaths == rhs.symbolPaths
            && sourcePaths == rhs.sourcePaths
            && breakEvents == rhs.breakEvents
            && breakFunctions == rhs.breakFunctions;
}

} // namespace Internal
} // namespace Debugger
