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
#ifndef VCPROJECTMANAGER_INTERNAL_PROJECTREFERENCE_H
#define VCPROJECTMANAGER_INTERNAL_PROJECTREFERENCE_H

#include "ivcprojectnodemodel.h"

#include "projectreference_private.h"

namespace VcProjectManager {
namespace Internal {

class ProjectReference : public IVcProjectXMLNode
{
    friend class ProjectReferenceFactory;

public:
    typedef QSharedPointer<ProjectReference>    Ptr;

    ~ProjectReference();

    void processNode(const QDomNode &node);
    void processNodeAttributes(const QDomElement &element);
    VcNodeWidget* createSettingsWidget();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    /*!
     * Implement in order to support creating a clone of a ProjectReference instance.
     * \return A shared pointer to newly created ProjectReference instance.
     */
    virtual ProjectReference::Ptr clone() const = 0;

    QString name() const;
    void setName(const QString &name);
    QString referencedProjectIdentifier() const;
    void setReferencedProjectIdentifier(const QString &referencedProjectIdentifier);
    void addReferenceConfiguration(ReferenceConfiguration::Ptr refConfig);
    void removeReferenceConfiguration(ReferenceConfiguration::Ptr refConfig);
    void removeReferenceConfiguration(const QString &refConfigName);
    QList<ReferenceConfiguration::Ptr> referenceConfigurations() const;
    ReferenceConfiguration::Ptr referenceConfiguration(const QString &refConfigName) const;

protected:
    ProjectReference();
    ProjectReference(const ProjectReference &projRef);
    ProjectReference& operator=(const ProjectReference &projRef);

    /*!
     * Called after instance of the ProjectReference is created in order to initialize \b m_private member variable to
     * a proper version of a ProjectReference_Private implementation (2003, 2005 or 2008).
     */
    virtual void init() = 0;

    ProjectReference_Private::Ptr m_private;
};

class ProjectReference2003 : public ProjectReference
{
    friend class ProjectReferenceFactory;

public:
    ProjectReference2003(const ProjectReference2003 &projRef);
    ProjectReference2003& operator=(const ProjectReference2003 &projRef);
    ~ProjectReference2003();

    ProjectReference::Ptr clone() const;

protected:
    ProjectReference2003();
    void init();
};

class ProjectReference2005 : public ProjectReference2003
{
    friend class ProjectReferenceFactory;

public:
    ProjectReference2005(const ProjectReference2005 &projRef);
    ProjectReference2005& operator=(const ProjectReference2005 &projRef);
    ~ProjectReference2005();

    ProjectReference::Ptr clone() const;

    QString copyLocal() const;
    void setCopyLocal(const QString &copyLocal);
    bool useInBuild() const;
    void setUseInBuild(bool useInBuild);
    QString relativePathFromSolution() const;
    void setRelativePathFromSolution(const QString &relativePathFromSolution);

protected:
    ProjectReference2005();
    void init();
};

class ProjectReference2008 : public ProjectReference2005
{
    friend class ProjectReferenceFactory;

public:
    ProjectReference2008(const ProjectReference2008 &projRef);
    ProjectReference2008& operator=(const ProjectReference2008 &projRef);
    ~ProjectReference2008();

    ProjectReference::Ptr clone() const;

    QString relativePathToProject() const;
    void setRelativePathToProject(const QString &relativePathToProject);
    bool useDependenciesInBuild() const;
    void setUseDependenciesInBuild(bool useDependenciesInBuild);
    bool copyLocalSatelliteAssemblies() const;
    void setCopyLocalSatelliteAssemblies(bool copyLocalSatelliteAssemblies);
    bool copyLocalDependencies() const;
    void setCopyLocalDependencies(bool copyLocalDependencies);

private:
    ProjectReference2008();
    void init();
};

class ProjectReferenceFactory
{
public:
    static ProjectReferenceFactory& instance();
    ProjectReference::Ptr create(VcDocConstants::DocumentVersion version);
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_PROJECTREFERENCE_H
