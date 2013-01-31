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

#include "qmlprojectfile.h"
#include "qmlproject.h"
#include "qmlprojectconstants.h"
#include <utils/qtcassert.h>

namespace QmlProjectManager {
namespace Internal {

QmlProjectFile::QmlProjectFile(QmlProject *parent, QString fileName)
    : Core::IDocument(parent),
      m_project(parent),
      m_fileName(fileName)
{
    QTC_CHECK(m_project);
    QTC_CHECK(!fileName.isEmpty());
}

QmlProjectFile::~QmlProjectFile()
{
}

bool QmlProjectFile::save(QString *, const QString &, bool)
{
    return false;
}

void QmlProjectFile::rename(const QString &newName)
{
    // Can't happen...
    Q_UNUSED(newName);
    Q_ASSERT(false);
}

QString QmlProjectFile::fileName() const
{
    return m_fileName;
}

QString QmlProjectFile::defaultPath() const
{
    return QString();
}

QString QmlProjectFile::suggestedFileName() const
{
    return QString();
}

QString QmlProjectFile::mimeType() const
{
    return QLatin1String(Constants::QMLPROJECT_MIMETYPE);
}

bool QmlProjectFile::isModified() const
{
    return false;
}

bool QmlProjectFile::isSaveAsAllowed() const
{
    return false;
}

Core::IDocument::ReloadBehavior QmlProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool QmlProjectFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)

    if (type == TypeContents)
        m_project->refreshProjectFile();

    return true;
}

} // namespace Internal
} // namespace QmlProjectManager
