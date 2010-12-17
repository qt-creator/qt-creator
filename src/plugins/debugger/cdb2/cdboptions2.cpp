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

#include "cdboptions2.h"
#include "cdbengine2.h"

#ifdef Q_OS_WIN
#    include <utils/winutils.h>
#endif

#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QCoreApplication>

static const char settingsGroupC[] = "CDB2";
static const char enabledKeyC[] = "Enabled";
static const char pathKeyC[] = "Path";
static const char symbolPathsKeyC[] = "SymbolPaths";
static const char sourcePathsKeyC[] = "SourcePaths";
static const char breakEventKeyC[] = "BreakEvent";
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

void CdbOptions::clearExecutable()
{
    is64bit = enabled = false;
    executable.clear();
}

void CdbOptions::clear()
{
    clearExecutable();
    symbolPaths.clear();
    sourcePaths.clear();
}

static inline QString msgAutoDetectFail(bool is64Bit, const QString &executable,
                                        const QString &extLib)
{
    return QCoreApplication::translate("Debugger::Cdb::CdbOptions",
        "Auto-detection of the new CDB debugging engine (%1bit) failed:\n"
        "Debugger executable: %2\n"
        "Extension library  : %3 not present.\n").arg(is64Bit ? 64 : 32).
         arg(QDir::toNativeSeparators(executable), QDir::toNativeSeparators(extLib));
}

static inline QString msgAutoDetect(bool is64Bit, const QString &executable,
                                    const QString &extLib,
                                    const QStringList &symbolPaths)
{
    return QCoreApplication::translate("Debugger::Cdb::CdbOptions",
        "The new CDB debugging engine (%1bit) has been set up automatically:\n"
        "Debugger executable: %2\n"
        "Extension library  : %3\n"
        "Symbol paths       : %4\n").arg(is64Bit ? 64 : 32).
        arg(QDir::toNativeSeparators(executable), QDir::toNativeSeparators(extLib),
        symbolPaths.join(QString(QLatin1Char(';'))));
}

QStringList CdbOptions::oldEngineSymbolPaths(const QSettings *s)
{
    return s->value(QLatin1String("CDB/SymbolPaths")).toStringList();
}

bool CdbOptions::autoDetect(const QSettings *s)
{
    QString autoExecutable;
    bool auto64Bit;
    // Check installation  and existence of the extension library
    CdbOptions::autoDetectExecutable(&autoExecutable, &auto64Bit);
    if (autoExecutable.isEmpty())
        return false;
    const QString extLib = CdbEngine::extensionLibraryName(auto64Bit);
    if (!QFileInfo(extLib).isFile()) {
        const QString failMsg = msgAutoDetectFail(auto64Bit, autoExecutable, extLib);
        qWarning("%s", qPrintable(failMsg));
          clearExecutable();
        return false;
    }
    enabled = true;
    is64bit = auto64Bit;
    executable = autoExecutable;
    // Is there a symbol path from an old install? Use that
    if (symbolPaths.empty())
        symbolPaths = CdbOptions::oldEngineSymbolPaths(s);
    const QString msg = msgAutoDetect(is64bit, QDir::toNativeSeparators(executable),
                                      QDir::toNativeSeparators(extLib), symbolPaths);
    qWarning("%s", qPrintable(msg));
    return true;
}

void CdbOptions::fromSettings(QSettings *s)
{
    clear();
    // Is this the first time we are called ->
    // try to find automatically
    const QString keyRoot = QLatin1String(settingsGroupC) + QLatin1Char('/');
    const QString enabledKey = keyRoot + QLatin1String(enabledKeyC);
    // First-time autodetection: Write back parameters
    const bool firstTime = !s->contains(enabledKey);
    if (firstTime && autoDetect(s)) {
        toSettings(s);
        return;
    }
    enabled = s->value(enabledKey, false).toBool();
    is64bit = s->value(keyRoot + QLatin1String(is64bitKeyC), is64bit).toBool();
    executable = s->value(keyRoot + QLatin1String(pathKeyC), executable).toString();
    symbolPaths = s->value(keyRoot + QLatin1String(symbolPathsKeyC), QStringList()).toStringList();
    sourcePaths = s->value(keyRoot + QLatin1String(sourcePathsKeyC), QStringList()).toStringList();
    breakEvents = s->value(keyRoot + QLatin1String(breakEventKeyC), QStringList()).toStringList();
}

void CdbOptions::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(enabledKeyC), enabled);
    s->setValue(QLatin1String(pathKeyC), executable);
    s->setValue(QLatin1String(is64bitKeyC), is64bit);
    s->setValue(QLatin1String(symbolPathsKeyC), symbolPaths);
    s->setValue(QLatin1String(sourcePathsKeyC), sourcePaths);
    s->setValue(QLatin1String(breakEventKeyC), breakEvents);
    s->endGroup();
}

bool CdbOptions::equals(const CdbOptions &rhs) const
{
    return enabled == rhs.enabled && is64bit == rhs.is64bit
            && executable == rhs.executable
            && symbolPaths == rhs.symbolPaths
            && sourcePaths == rhs.sourcePaths
            && breakEvents == rhs.breakEvents;
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
