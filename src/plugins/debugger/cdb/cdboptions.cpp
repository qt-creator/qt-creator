/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cdboptions.h"

#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

static const char *settingsGroupC = "CDB";
static const char *enabledKeyC = "enabled";
static const char *pathKeyC = "path";

namespace Debugger {
namespace Internal {

CdbOptions::CdbOptions() :
    enabled(false)
{
}

void CdbOptions::clear()
{
    enabled = false;
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
        enabled = autoDetectPath(&path);
        return;
    }
    enabled = s->value(enabledKey, false).toBool();
    path = s->value(keyRoot + QLatin1String(pathKeyC), QString()).toString();
}

void CdbOptions::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(settingsGroupC));
    s->setValue(QLatin1String(enabledKeyC), enabled);
    s->setValue(QLatin1String(pathKeyC), path);
    s->endGroup();
}

bool CdbOptions::autoDetectPath(QString *outPath)
{
    // Look for $ProgramFiles/"Debugging Tools For Windows" and its
    // :" (x86)", " (x64)" variations
    static const char *postFixes[] = { " (x86)", " (x64)" };

    outPath->clear();
    const QByteArray programDirB = qgetenv("ProgramFiles");
    if (programDirB.isEmpty())
        return false;

    const QString programDir = QString::fromLocal8Bit(programDirB) + QDir::separator();
    const QString installDir = QLatin1String("Debugging Tools For Windows");
    QString path = programDir + installDir;
    if (QFileInfo(path).isDir()) {
        *outPath = QDir::toNativeSeparators(path);
        return true;
    }
    const int rootLength = path.size();
    for (int i = 0; i < sizeof(postFixes)/sizeof(const char*); i++) {
        path.truncate(rootLength);
        path += QLatin1String(postFixes[i]);
        if (QFileInfo(path).isDir()) {
            *outPath = QDir::toNativeSeparators(path);
            return true;
        }
    }
    return false;
}

bool CdbOptions::equals(const CdbOptions &rhs) const
{
    return enabled == rhs.enabled && path == rhs.path;
}

} // namespace Internal
} // namespace Debugger
