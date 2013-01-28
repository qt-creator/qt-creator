/**************************************************************************
**
** Copyright (C) 2013 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#include "autotoolsprojectfile.h"
#include "autotoolsproject.h"
#include "autotoolsprojectconstants.h"

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Internal;

AutotoolsProjectFile::AutotoolsProjectFile(AutotoolsProject *project, const QString &fileName) :
    Core::IDocument(project),
    m_project(project),
    m_fileName(fileName)
{
}

bool AutotoolsProjectFile::save(QString *errorString, const QString &fileName, bool autoSave)
{
    Q_UNUSED(errorString);
    Q_UNUSED(fileName);
    Q_UNUSED(autoSave);

    return false;
}

QString AutotoolsProjectFile::fileName() const
{
    return m_fileName;
}

QString AutotoolsProjectFile::defaultPath() const
{
    return QString();
}

QString AutotoolsProjectFile::suggestedFileName() const
{
    return QString();
}

QString AutotoolsProjectFile::mimeType() const
{
    return QLatin1String(Constants::MAKEFILE_MIMETYPE);
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

void AutotoolsProjectFile::rename(const QString &newName)
{
    Q_UNUSED(newName);
}
