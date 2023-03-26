// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectcontroller.h"

#include "qmt/project/project.h"
#include "qmt/serializer/projectserializer.h"

#include "qmt/model/mpackage.h"

#include "../../modelinglibtr.h"

namespace qmt {

NoFileNameException::NoFileNameException()
    : Exception(Tr::tr("Missing file name."))
{
}

ProjectIsModifiedException::ProjectIsModifiedException()
    : Exception(Tr::tr("Project is modified."))
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
    rootPackage->setName(Tr::tr("Model"));
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
