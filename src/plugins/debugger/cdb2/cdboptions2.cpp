/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cdboptions2.h"

#ifdef Q_OS_WIN
#    include <utils/winutils.h>
#endif

#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

static const char settingsGroupC[] = "CDB2";
static const char enabledKeyC[] = "Enabled";
static const char pathKeyC[] = "Path";
static const char symbolPathsKeyC[] = "SymbolPaths";
static const char sourcePathsKeyC[] = "SourcePaths";
static const char is64bitKeyC[] = "64bit";

namespace Debugger {
namespace Cdb {

CdbOptions::CdbOptions() :
    enabled(false), is64bit(false)
{
}

QString CdbOptions::settingsGroup()
{
    return QLatin1String(settingsGroupC);
}

void CdbOptions::clear()
{
    is64bit = enabled = false;
    executable.clear();
    symbolPaths.clear();
    sourcePaths.clear();
}

void CdbOptions::fromSettings(const QSettings *s)
{
    clear();
    // Is this the first time we are called ->
    // try to find automatically
    const QString keyRoot = QLatin1String(settingsGroupC) + QLatin1Char('/');
    const QString enabledKey = keyRoot + QLatin1String(enabledKeyC);
#if 0 // TODO: Enable autodetection after deprecating the old CDB engine only.
    const bool firstTime = !s->contains(enabledKey);
    if (firstTime)
        CdbOptions::autoDetectExecutable(&executable, &is64bit);
#endif
    enabled = s->value(enabledKey, false).toBool();
    is64bit = s->value(keyRoot + QLatin1String(is64bitKeyC), is64bit).toBool();
    executable = s->value(keyRoot + QLatin1String(pathKeyC), executable).toString();
    symbolPaths = s->value(keyRoot + QLatin1String(symbolPathsKeyC), QStringList()).toStringList();
    sourcePaths = s->value(keyRoot + QLatin1String(sourcePathsKeyC), QStringList()).toStringList();
}

void CdbOptions::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(enabledKeyC), enabled);
    s->setValue(QLatin1String(pathKeyC), executable);
    s->setValue(QLatin1String(is64bitKeyC), is64bit);
    s->setValue(QLatin1String(symbolPathsKeyC), symbolPaths);
    s->setValue(QLatin1String(sourcePathsKeyC), sourcePaths);
    s->endGroup();
}

bool CdbOptions::equals(const CdbOptions &rhs) const
{
    return enabled == rhs.enabled && is64bit == rhs.is64bit
            && executable == rhs.executable
            && symbolPaths == rhs.symbolPaths
            && sourcePaths == rhs.sourcePaths;
}

bool CdbOptions::autoDetectExecutable(QString *outPath, bool *is64bitIn  /* = 0 */,
                                      QStringList *checkedDirectories /* = 0 */)
{
    // Look for $ProgramFiles/"Debugging Tools For Windows <bit-idy>/cdb.exe" and its
    // " (x86)", " (x64)" variations.
    static const char *postFixes[] = {" (x64)", " 64-bit", " (x86)", " (x32)" };
    enum { first32bitIndex = 2 };

    if (checkedDirectories)
        checkedDirectories->clear();

    outPath->clear();
    const QByteArray programDirB = qgetenv("ProgramFiles");
    if (programDirB.isEmpty())
        return false;

    const QString programDir = QString::fromLocal8Bit(programDirB) + QLatin1Char('/');
    const QString installDir = QLatin1String("Debugging Tools For Windows");
    const QString executable = QLatin1String("/cdb.exe");

    QString path = programDir + installDir;
    if (checkedDirectories)
        checkedDirectories->push_back(path);
    const QFileInfo fi(path + executable);
    // Plain system installation
    if (fi.isFile() && fi.isExecutable()) {
        *outPath = fi.absoluteFilePath();
        if (is64bitIn)
#ifdef Q_OS_WIN
            *is64bitIn = Utils::winIs64BitSystem();
#else
            *is64bitIn = false;
#endif
        return true;
    }
    // Try the post fixes
    const int rootLength = path.size();
    for (unsigned i = 0; i < sizeof(postFixes)/sizeof(const char*); i++) {
        path.truncate(rootLength);
        path += QLatin1String(postFixes[i]);
        if (checkedDirectories)
            checkedDirectories->push_back(path);
        const QFileInfo fi2(path + executable);
        if (fi2.isFile() && fi2.isExecutable()) {
            if (is64bitIn)
                *is64bitIn = i < first32bitIndex;
            *outPath = fi2.absoluteFilePath();
            return true;
        }
    }
    return false;
}

} // namespace Internal
} // namespace Debugger
