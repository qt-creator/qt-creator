/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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
#include "projectreference.h"

#include "referenceconfiguration.h"

namespace VcProjectManager {
namespace Internal {

ProjectReference::~ProjectReference()
{
}

void ProjectReference::processNode(const QDomNode &node)
{
    m_private->processNode(node);
}

void ProjectReference::processNodeAttributes(const QDomElement &element)
{
    Q_UNUSED(element)
}

VcNodeWidget *ProjectReference::createSettingsWidget()
{
    // TODO(Radovan): Finish implementation
    return 0;
}

QDomNode ProjectReference::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    return m_private->toXMLDomNode(domXMLDocument);
}

QString ProjectReference::name() const
{
    return m_private->name();
}

void ProjectReference::setName(const QString &name)
{
    m_private->setName(name);
}

QString ProjectReference::referencedProjectIdentifier() const
{
    return m_private->referencedProjectIdentifier();
}

void ProjectReference::setReferencedProjectIdentifier(const QString &referencedProjectIdentifier)
{
    m_private->setReferencedProjectIdentifier(referencedProjectIdentifier);
}

void ProjectReference::addReferenceConfiguration(ReferenceConfiguration::Ptr refConfig)
{
    m_private->addReferenceConfiguration(refConfig);
}

void ProjectReference::removeReferenceConfiguration(ReferenceConfiguration::Ptr refConfig)
{
    m_private->removeReferenceConfiguration(refConfig);
}

void ProjectReference::removeReferenceConfiguration(const QString &refConfigName)
{
    m_private->removeReferenceConfiguration(refConfigName);
}

QList<ReferenceConfiguration::Ptr> ProjectReference::referenceConfigurations() const
{
    return m_private->referenceConfigurations();
}

ReferenceConfiguration::Ptr ProjectReference::referenceConfiguration(const QString &refConfigName) const
{
    return m_private->referenceConfiguration(refConfigName);
}

ProjectReference::ProjectReference()
{
}

ProjectReference::ProjectReference(const ProjectReference &projRef)
{
    m_private = projRef.m_private->clone();
}

ProjectReference &ProjectReference::operator =(const ProjectReference &projRef)
{
    if (this != &projRef)
        m_private = projRef.m_private->clone();
    return *this;
}


ProjectReference2003::ProjectReference2003(const ProjectReference2003 &projRef)
    : ProjectReference(projRef)
{
}

ProjectReference2003 &ProjectReference2003::operator =(const ProjectReference2003 &projRef)
{
    ProjectReference::operator =(projRef);
    return *this;
}

ProjectReference2003::~ProjectReference2003()
{
}

ProjectReference::Ptr ProjectReference2003::clone() const
{
    return ProjectReference::Ptr(new ProjectReference2003(*this));
}

ProjectReference2003::ProjectReference2003()
{
}

void ProjectReference2003::init()
{
    m_private = ProjectReference_Private::Ptr(new ProjectReference2003_Private);
}


ProjectReference2005::ProjectReference2005(const ProjectReference2005 &projRef)
    : ProjectReference2003(projRef)
{
}

ProjectReference2005 &ProjectReference2005::operator =(const ProjectReference2005 &projRef)
{
    ProjectReference2003::operator =(projRef);
    return *this;
}

ProjectReference2005::~ProjectReference2005()
{
}

ProjectReference::Ptr ProjectReference2005::clone() const
{
    return ProjectReference::Ptr(new ProjectReference2005(*this));
}

QString ProjectReference2005::copyLocal() const
{
    QSharedPointer<ProjectReference2005_Private> ref = m_private.staticCast<ProjectReference2005_Private>();
    return ref->copyLocal();
}

void ProjectReference2005::setCopyLocal(const QString &copyLocal)
{
    QSharedPointer<ProjectReference2005_Private> ref = m_private.staticCast<ProjectReference2005_Private>();
    ref->setCopyLocal(copyLocal);
}

bool ProjectReference2005::useInBuild() const
{
    QSharedPointer<ProjectReference2005_Private> ref = m_private.staticCast<ProjectReference2005_Private>();
    return ref->useInBuild();
}

void ProjectReference2005::setUseInBuild(bool useInBuild)
{
    QSharedPointer<ProjectReference2005_Private> ref = m_private.staticCast<ProjectReference2005_Private>();
    ref->setUseInBuild(useInBuild);
}

QString ProjectReference2005::relativePathFromSolution() const
{
    QSharedPointer<ProjectReference2005_Private> ref = m_private.staticCast<ProjectReference2005_Private>();
    return ref->relativePathFromSolution();
}

void ProjectReference2005::setRelativePathFromSolution(const QString &relativePathFromSolution)
{
    QSharedPointer<ProjectReference2005_Private> ref = m_private.staticCast<ProjectReference2005_Private>();
    ref->setRelativePathFromSolution(relativePathFromSolution);
}

ProjectReference2005::ProjectReference2005()
{
}

void ProjectReference2005::init()
{
    m_private = ProjectReference_Private::Ptr(new ProjectReference2005_Private);
}


ProjectReference2008::ProjectReference2008(const ProjectReference2008 &projRef)
    : ProjectReference2005(projRef)
{
}

ProjectReference2008 &ProjectReference2008::operator =(const ProjectReference2008 &projRef)
{
    ProjectReference2005::operator =(projRef);
    return *this;
}

ProjectReference2008::~ProjectReference2008()
{
}

ProjectReference::Ptr ProjectReference2008::clone() const
{
    return ProjectReference::Ptr(new ProjectReference2008(*this));
}

QString ProjectReference2008::relativePathToProject() const
{
    QSharedPointer<ProjectReference2008_Private> ref = m_private.staticCast<ProjectReference2008_Private>();
    return ref->relativePathToProject();
}

void ProjectReference2008::setRelativePathToProject(const QString &relativePathToProject)
{
    QSharedPointer<ProjectReference2008_Private> ref = m_private.staticCast<ProjectReference2008_Private>();
    ref->setRelativePathToProject(relativePathToProject);
}

bool ProjectReference2008::useDependenciesInBuild() const
{
    QSharedPointer<ProjectReference2008_Private> ref = m_private.staticCast<ProjectReference2008_Private>();
    return ref->useDependenciesInBuild();
}

void ProjectReference2008::setUseDependenciesInBuild(bool useDependenciesInBuild)
{
    QSharedPointer<ProjectReference2008_Private> ref = m_private.staticCast<ProjectReference2008_Private>();
    ref->setUseDependenciesInBuild(useDependenciesInBuild);
}

bool ProjectReference2008::copyLocalSatelliteAssemblies() const
{
    QSharedPointer<ProjectReference2008_Private> ref = m_private.staticCast<ProjectReference2008_Private>();
    return ref->copyLocalSatelliteAssemblies();
}

void ProjectReference2008::setCopyLocalSatelliteAssemblies(bool copyLocalSatelliteAssemblies)
{
    QSharedPointer<ProjectReference2008_Private> ref = m_private.staticCast<ProjectReference2008_Private>();
    ref->setCopyLocalSatelliteAssemblies(copyLocalSatelliteAssemblies);
}

bool ProjectReference2008::copyLocalDependencies() const
{
    QSharedPointer<ProjectReference2008_Private> ref = m_private.staticCast<ProjectReference2008_Private>();
    return ref->copyLocalDependencies();
}

void ProjectReference2008::setCopyLocalDependencies(bool copyLocalDependencies)
{
    QSharedPointer<ProjectReference2008_Private> ref = m_private.staticCast<ProjectReference2008_Private>();
    ref->setCopyLocalDependencies(copyLocalDependencies);
}

ProjectReference2008::ProjectReference2008()
{
}

void ProjectReference2008::init()
{
    m_private = ProjectReference_Private::Ptr(new ProjectReference2008_Private);
}


ProjectReferenceFactory &ProjectReferenceFactory::instance()
{
    static ProjectReferenceFactory pr;
    return pr;
}

ProjectReference::Ptr ProjectReferenceFactory::create(VcDocConstants::DocumentVersion version)
{
    ProjectReference::Ptr ref;

    switch (version) {
    case VcDocConstants::DV_MSVC_2003:
        ref = ProjectReference::Ptr(new ProjectReference2003);
        break;
    case VcDocConstants::DV_MSVC_2005:
        ref = ProjectReference::Ptr(new ProjectReference2005);
        break;
    case VcDocConstants::DV_MSVC_2008:
        ref = ProjectReference::Ptr(new ProjectReference2008);
        break;
    }

    if (ref)
        ref->init();

    return ref;
}

} // namespace Internal
} // namespace VcProjectManager
