/**************************************************************************
**
** Copyright (C) 2015 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#include "autotoolsprojectfile.h"
#include "autotoolsproject.h"
#include "autotoolsprojectconstants.h"

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;

AutotoolsProjectFile::AutotoolsProjectFile(AutotoolsProject *project, const QString &fileName) :
    Core::IDocument(project),
    m_project(project)
{
    setId("Autotools.ProjectFile");
    setMimeType(QLatin1String(Constants::MAKEFILE_MIMETYPE));
    setFilePath(Utils::FileName::fromString(fileName));
}

bool AutotoolsProjectFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    Q_UNUSED(errorString);
    Q_UNUSED(fileName);
    Q_UNUSED(autoSave);

    return false;
}

QString AutotoolsProjectFile::defaultPath() const
{
    return QString();
}

QString AutotoolsProjectFile::suggestedFileName() const
{
    return QString();
}

bool AutotoolsProjectFile::isModified() const
{
    return false;
}

bool AutotoolsProjectFile::isSaveAsAllowed() const
{
    return false;
}

bool AutotoolsProjectFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString);
    Q_UNUSED(flag);
    Q_UNUSED(type);

    return false;
}
