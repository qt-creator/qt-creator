/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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
    : QObject(parent),
      m_isModified(false)
{
}

ProjectController::~ProjectController()
{
}

void ProjectController::newProject(const QString &file_name)
{
    m_project.reset(new Project());
    MPackage *root_package = new MPackage();
    root_package->setName(tr("Model"));
    m_project->setRootPackage(root_package);
    m_project->setFileName(file_name);
    m_isModified = false;
    emit fileNameChanged(m_project->getFileName());
    emit changed();
}

void ProjectController::setFileName(const QString &file_name)
{
    if (file_name != m_project->getFileName()) {
        m_project->setFileName(file_name);
        setModified();
        emit fileNameChanged(m_project->getFileName());
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
    if (isModified()) {
        throw ProjectIsModifiedException();
    }
    if (!m_project->hasFileName()) {
        throw NoFileNameException();
    }
    ProjectSerializer serializer;
    serializer.load(m_project->getFileName(), m_project.data());
    m_isModified = false;
    emit changed();
}

void ProjectController::save()
{
    if (!m_project->hasFileName()) {
        throw NoFileNameException();
    }
    ProjectSerializer serializer;
    serializer.save(m_project->getFileName(), m_project.data());
    m_isModified = false;
    emit changed();
}

void ProjectController::saveAs(const QString &file_name)
{
    setFileName(file_name);
    save();
}

}
