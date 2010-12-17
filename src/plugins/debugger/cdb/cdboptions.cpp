/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cdboptions.h"
#include "coreengine.h"
#include "cdbsymbolpathlisteditor.h"

#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

static const char *settingsGroupC = "CDB";
static const char *enabledKeyC = "Enabled";
static const char *pathKeyC = "Path";
static const char *symbolPathsKeyC = "SymbolPaths";
static const char *sourcePathsKeyC = "SourcePaths";
static const char *breakOnExceptionKeyC = "BreakOnException";
static const char *verboseSymbolLoadingKeyC = "VerboseSymbolLoading";
static const char *fastLoadDebuggingHelpersKeyC = "FastLoadDebuggingHelpers";

namespace Debugger {
namespace Internal {

CdbOptions::CdbOptions() :
    enabled(false),
    breakOnException(false),
    verboseSymbolLoading(false),
    fastLoadDebuggingHelpers(true)
{
}

QString CdbOptions::settingsGroup()
{
    return QLatin1String(settingsGroupC);
}

void CdbOptions::clear()
{
    enabled = false;
    verboseSymbolLoading = false;
    fastLoadDebuggingHelpers = true;
    path.clear();
}

void CdbOptions::fromSettings(const QSettings *s)
{
    clear();
    // Is this the first time we are called ->
    // try to find automatically
    const QString keyRoot = QLatin1String(settingsGroupC) + QLatin1Char('/');
    const QString enabledKey = keyRoot + QLatin1String(enabledKeyC);
    const bool firstTime = !s->contains(enabledKey);
    if (firstTime) {
        enabled = CdbCore::CoreEngine::autoDetectPath(&path);
        return;
    }
    enabled = s->value(enabledKey, false).toBool();
    path = s->value(keyRoot + QLatin1String(pathKeyC), QString()).toString();
    symbolPaths = s->value(keyRoot + QLatin1String(symbolPathsKeyC), QStringList()).toStringList();
    sourcePaths = s->value(keyRoot + QLatin1String(sourcePathsKeyC), QStringList()).toStringList();
    verboseSymbolLoading = s->value(keyRoot + QLatin1String(verboseSymbolLoadingKeyC), false).toBool();
    fastLoadDebuggingHelpers = s->value(keyRoot + QLatin1String(fastLoadDebuggingHelpersKeyC), true).toBool();
    breakOnException = s->value(keyRoot + QLatin1String(breakOnExceptionKeyC), false).toBool();
}

void CdbOptions::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(enabledKeyC), enabled);
    s->setValue(QLatin1String(pathKeyC), path);
    s->setValue(QLatin1String(symbolPathsKeyC), symbolPaths);
    s->setValue(QLatin1String(sourcePathsKeyC), sourcePaths);
    s->setValue(QLatin1String(verboseSymbolLoadingKeyC), verboseSymbolLoading);
    s->setValue(QLatin1String(fastLoadDebuggingHelpersKeyC), fastLoadDebuggingHelpers);
    s->setValue(QLatin1String(breakOnExceptionKeyC), breakOnException);
    s->endGroup();
}

unsigned CdbOptions::compare(const CdbOptions &rhs) const
{
    unsigned rc = 0;
    if (enabled != rhs.enabled || path != rhs.path)
        rc |= InitializationOptionsChanged;
    if (symbolPaths != rhs.symbolPaths || sourcePaths != rhs.sourcePaths)
        rc |= DebuggerPathsChanged;
    if (verboseSymbolLoading != rhs.verboseSymbolLoading)
        rc |= SymbolOptionsChanged;
    if (fastLoadDebuggingHelpers != rhs.fastLoadDebuggingHelpers)
        rc |= FastLoadDebuggingHelpersChanged;
    if (breakOnException != rhs.breakOnException)
        rc |= OtherOptionsChanged;
    return rc;
}

QString CdbOptions::symbolServerPath(const QString &cacheDir)
{
    return CdbSymbolPathListEditor::symbolServerPath(cacheDir);
}

int CdbOptions::indexOfSymbolServerPath(const QStringList &symbolPaths, QString *cacheDir)
{
    return CdbSymbolPathListEditor::indexOfSymbolServerPath(symbolPaths, cacheDir);
}

} // namespace Internal
} // namespace Debugger
