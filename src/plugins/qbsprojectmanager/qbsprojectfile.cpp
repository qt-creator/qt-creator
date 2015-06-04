/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qbsprojectfile.h"

#include "qbsproject.h"
#include "qbsprojectmanagerconstants.h"

namespace QbsProjectManager {
namespace Internal {

QbsProjectFile::QbsProjectFile(QbsProject *parent, QString fileName) : Core::IDocument(parent),
    m_project(parent)
{
    setId("Qbs.ProjectFile");
    setMimeType(QLatin1String(Constants::MIME_TYPE));
    setFilePath(Utils::FileName::fromString(fileName));
}

bool QbsProjectFile::save(QString *, const QString &, bool)
{
    return false;
}

QString QbsProjectFile::defaultPath() const
{
    return QString();
}

QString QbsProjectFile::suggestedFileName() const
{
    return QString();
}

bool QbsProjectFile::isModified() const
{
    return false;
}

bool QbsProjectFile::isSaveAsAllowed() const
{
    return false;
}

Core::IDocument::ReloadBehavior QbsProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state);
    Q_UNUSED(type);
    return BehaviorSilent;
}

bool QbsProjectFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    if (type == TypePermissions)
        return true;
    m_project->delayParsing();
    return true;
}

} // namespace Internal
} // namespace QbsProjectManager

