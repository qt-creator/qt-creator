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
#ifndef VCPROJECTMANAGER_INTERNAL_PROJECTREFERENCE_PRIVATE_H
#define VCPROJECTMANAGER_INTERNAL_PROJECTREFERENCE_PRIVATE_H

#include "referenceconfiguration.h"
#include "vcprojectdocument_constants.h"

namespace VcProjectManager {
namespace Internal {

class ProjectReference_Private
{
    friend class ProjectReference;

public:
    typedef QSharedPointer<ProjectReference_Private>    Ptr;

    virtual ~ProjectReference_Private();
    virtual void processNode(const QDomNode &node);
    virtual QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    /*!
     * Implement in order to support creating a clone of a ProjectReference_Private instance.
     * \return A shared pointer to newly created ProjectReference_Private instance.
     */
    virtual ProjectReference_Private::Ptr clone() const = 0;

    QString name() const;
    void setName(const QString &name);
    QString referencedProjectIdentifier() const;
    void setReferencedProjectIdentifier(const QString &referencedProjectIdentifier);
    void addReferenceConfiguration(ReferenceConfiguration::Ptr refConfig);
    void removeReferenceConfiguration(ReferenceConfiguration::Ptr refConfig);
    void removeReferenceConfiguration(const QString &refConfigName);
    QList<ReferenceConfiguration::Ptr > referenceConfigurations() const;
    ReferenceConfiguration::Ptr referenceConfiguration(const QString &refConfigName) const;

protected:
    ProjectReference_Private();
    ProjectReference_Private(const ProjectReference_Private &projRef_p);
    ProjectReference_Private& operator=(const ProjectReference_Private &projRef_p);
    virtual void processReferenceConfig(const QDomNode &referenceConfig);
    virtual void processNodeAttributes(const QDomElement &element);

    /*!
     * Reimplement this to create a new reference configuration.
     * \return A shared pointer to a newly created reference configuration.
     */
    virtual ReferenceConfiguration::Ptr createReferenceConfiguration() const = 0;

    QList<ReferenceConfiguration::Ptr > m_referenceConfigurations;
    QString m_referencedProjectIdentifier; // required
    QString m_name; // optional
};

class ProjectReference2003_Private : public ProjectReference_Private
{
    friend class ProjectReference2003;

public:
    ~ProjectReference2003_Private();
    ProjectReference_Private::Ptr clone() const;

protected:
    ProjectReference2003_Private();
    ProjectReference2003_Private(const ProjectReference2003_Private &projRef_p);
    ProjectReference2003_Private& operator=(const ProjectReference2003_Private &projRef_p);
    ReferenceConfiguration::Ptr createReferenceConfiguration() const;
};

class ProjectReference2005_Private : public ProjectReference2003_Private
{
    friend class ProjectReference2005;

public:
    ~ProjectReference2005_Private();
    virtual QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    ProjectReference_Private::Ptr clone() const;

    QString copyLocal() const;
    void setCopyLocal(const QString &copyLocal);
    bool useInBuild() const;
    void setUseInBuild(bool useInBuild);
    QString relativePathFromSolution() const;
    void setRelativePathFromSolution(const QString &relativePathFromSolution);

protected:
    ProjectReference2005_Private();
    ProjectReference2005_Private(const ProjectReference2005_Private &projRef_p);
    ProjectReference2005_Private& operator=(const ProjectReference2005_Private &projRef_p);
    void processNodeAttributes(const QDomElement &element);
    ReferenceConfiguration::Ptr createReferenceConfiguration() const;

    QString m_copyLocal; // optional
    bool m_useInBuild; // optional
    QString m_relativePathFromSolution; // optional
};

class ProjectReference2008_Private : public ProjectReference2005_Private
{
    friend class ProjectReference2008;

public:
    ~ProjectReference2008_Private();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    ProjectReference_Private::Ptr clone() const;

    QString relativePathToProject() const;
    void setRelativePathToProject(const QString &relativePathToProject);
    bool useDependenciesInBuild() const;
    void setUseDependenciesInBuild(bool useDependenciesInBuild);
    bool copyLocalSatelliteAssemblies() const;
    void setCopyLocalSatelliteAssemblies(bool copyLocalSatelliteAssemblies);
    bool copyLocalDependencies() const;
    void setCopyLocalDependencies(bool copyLocalDependencies);

protected:
    ProjectReference2008_Private();
    ProjectReference2008_Private(const ProjectReference2008_Private &projRef_p);
    ProjectReference2008_Private& operator=(const ProjectReference2008_Private &projRef_p);
    void processNodeAttributes(const QDomElement &element);
    ReferenceConfiguration::Ptr createReferenceConfiguration() const;

    QString m_relativePathToProject; // optional
    bool m_useDependenciesInBuild; //optional   default: true
    bool m_copyLocalSatelliteAssemblies; //optional     default: true
    bool m_copyLocalDependencies; //optional    default: true
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_PROJECTREFERENCE_PRIVATE_H
