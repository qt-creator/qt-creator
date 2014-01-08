/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "iversioncontrol.h"

namespace Core {

QString IVersionControl::vcsOpenText() const
{
    return tr("Open with VCS (%1)").arg(displayName());
}

QString IVersionControl::vcsMakeWritableText() const
{
    return QString();
}

QString IVersionControl::vcsTopic(const QString &)
{
    return QString();
}

IVersionControl::OpenSupportMode IVersionControl::openSupportMode(const QString &fileName) const
{
    Q_UNUSED(fileName);
    return NoOpen;
}

} // namespace Core

#if defined(WITH_TESTS)

#include "vcsmanager.h"

#include <QFileInfo>

namespace Core {

TestVersionControl::~TestVersionControl()
{
    VcsManager::instance()->clearVersionControlCache();
}

void TestVersionControl::setManagedDirectories(const QHash<QString, QString> &dirs)
{
    m_managedDirs = dirs;
    m_dirCount = 0;
    VcsManager::instance()->clearVersionControlCache();
}

void TestVersionControl::setManagedFiles(const QSet<QString> &files)
{
    m_managedFiles = files;
    m_fileCount = 0;
    VcsManager::instance()->clearVersionControlCache();
}

bool TestVersionControl::managesDirectory(const QString &filename, QString *topLevel) const
{
    ++m_dirCount;

    if (m_managedDirs.contains(filename)) {
        if (topLevel)
            *topLevel = m_managedDirs.value(filename);
        return true;
    }
    return false;
}

bool TestVersionControl::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    ++m_fileCount;

    QFileInfo fi(workingDirectory + QLatin1Char('/') + fileName);
    QString dir = fi.absolutePath();
    if (!managesDirectory(dir, 0))
        return false;
    QString file = fi.absoluteFilePath();
    return m_managedFiles.contains(file);
}

} // namespace Core
#endif
