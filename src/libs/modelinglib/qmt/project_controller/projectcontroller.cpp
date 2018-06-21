/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "projectcontroller.h"

#include "qmt/project/project.h"
#include "qmt/serializer/projectserializer.h"

#include "qmt/model/mpackage.h"

namespace qmt {

NoFileNameException::NoFileNameException()
    : Exception(ProjectController::tr("Missing file name."))
{
}

ProjectIsModifiedException::ProjectIsModifiedException()
    : Exception(ProjectController::tr("Project is modified."))
{
}

ProjectController::ProjectController(QObject *parent)
    : QObject(parent)
{
}

ProjectController::~ProjectController()
{
}

void ProjectController::newProject(const QString &fileName)
{
    m_project.reset(new Project());
    auto rootPackage = new MPackage();
    rootPackage->setName(tr("Model"));
    m_project->setRootPackage(rootPackage);
    m_project->setFileName(fileName);
    m_isModified = false;
    emit fileNameChanged(m_project->fileName());
    emit changed();
}

void ProjectController::setFileName(const QString &fileName)
{
    if (fileName != m_project->fileName()) {
        m_project->setFileName(fileName);
        setModified();
        emit fileNameChanged(m_project->fileName());
    }
}

void ProjectController::setModified()
{
    if (!m_isModified) {
        m_isModified = true;
        emit changed();
    }
}

void ProjectController::load()
{
    if (isModified())
        throw ProjectIsModifiedException();
    if (!m_project->hasFileName())
        throw NoFileNameException();
    ProjectSerializer serializer;
    serializer.load(m_project->fileName(), m_project.data());
    m_isModified = false;
    emit changed();
}

void ProjectController::save()
{
    if (!m_project->hasFileName())
        throw NoFileNameException();
    ProjectSerializer serializer;
    serializer.save(m_project->fileName(), m_project.data());
    m_isModified = false;
    emit changed();
}

void ProjectController::saveAs(const QString &fileName)
{
    setFileName(fileName);
    save();
}

} // namespace qmt
