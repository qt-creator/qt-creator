/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#include "projectreference_private.h"

#include <QVariant>

#include "configurationsfactory.h"

namespace VcProjectManager {
namespace Internal {

ProjectReference_Private::~ProjectReference_Private()
{
    m_referenceConfigurations.clear();
}

void ProjectReference_Private::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeType() == QDomNode::ElementNode)
        processNodeAttributes(node.toElement());

    if (node.hasChildNodes()) {
        QDomNode firstChild = node.firstChild();
        if (!firstChild.isNull())
            processReferenceConfig(firstChild);
    }
}

QDomNode ProjectReference_Private::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement projRefNode = domXMLDocument.createElement(QLatin1String("ProjectReference"));

    projRefNode.setAttribute(QLatin1String("Name"), m_name);
    projRefNode.setAttribute(QLatin1String("ReferencedProjectIdentifier"), m_referencedProjectIdentifier);

    foreach (const Configuration::Ptr &refConfig, m_referenceConfigurations)
        projRefNode.appendChild(refConfig->toXMLDomNode(domXMLDocument));

    return projRefNode;
}

QString ProjectReference_Private::name() const
{
    return m_name;
}

void ProjectReference_Private::setName(const QString &name)
{
    m_name = name;
}

QString ProjectReference_Private::referencedProjectIdentifier() const
{
    return m_referencedProjectIdentifier;
}

void ProjectReference_Private::setReferencedProjectIdentifier(const QString &referencedProjectIdentifier)
{
    m_referencedProjectIdentifier = referencedProjectIdentifier;
}

void ProjectReference_Private::addReferenceConfiguration(Configuration::Ptr refConfig)
{
    if (m_referenceConfigurations.contains(refConfig))
        return;

    // Don't add configuration with the same name
    foreach (const Configuration::Ptr &refConf, m_referenceConfigurations) {
        if (refConfig->name() == refConf->name())
            return;
    }
    m_referenceConfigurations.append(refConfig);
}

void ProjectReference_Private::removeReferenceConfiguration(Configuration::Ptr refConfig)
{
    m_referenceConfigurations.removeAll(refConfig);
}

void ProjectReference_Private::removeReferenceConfiguration(const QString &refConfigName)
{
    foreach (const Configuration::Ptr &refConfig, m_referenceConfigurations) {
        if (refConfig->name() == refConfigName) {
            removeReferenceConfiguration(refConfig);
            return;
        }
    }
}

QList<Configuration::Ptr> ProjectReference_Private::referenceConfigurations() const
{
    return m_referenceConfigurations;
}

Configuration::Ptr ProjectReference_Private::referenceConfiguration(const QString &refConfigName) const
{
    foreach (const Configuration::Ptr &refConfig, m_referenceConfigurations) {
        if (refConfig->name() == refConfigName)
            return refConfig;
    }
    return Configuration::Ptr();
}

ProjectReference_Private::ProjectReference_Private()
{
}

ProjectReference_Private::ProjectReference_Private(const ProjectReference_Private &projRef_p)
{
    m_referencedProjectIdentifier = projRef_p.m_referencedProjectIdentifier;
    m_name = projRef_p.m_name;

    foreach (const Configuration::Ptr &refConfig, projRef_p.m_referenceConfigurations)
        m_referenceConfigurations.append(refConfig->clone());
}

ProjectReference_Private &ProjectReference_Private::operator =(const ProjectReference_Private &projRef_p)
{
    if (this != &projRef_p) {
        m_referencedProjectIdentifier = projRef_p.m_referencedProjectIdentifier;
        m_name = projRef_p.m_name;

        m_referenceConfigurations.clear();
        foreach (const Configuration::Ptr &refConfig, projRef_p.m_referenceConfigurations)
            m_referenceConfigurations.append(refConfig->clone());
    }
    return *this;
}

void ProjectReference_Private::processReferenceConfig(const QDomNode &referenceConfig)
{
    Configuration::Ptr referenceConfiguration = createReferenceConfiguration();
    referenceConfiguration->processNode(referenceConfig);
    m_referenceConfigurations.append(referenceConfiguration);

    // process next sibling
    QDomNode nextSibling = referenceConfig.nextSibling();
    if (!nextSibling.isNull())
        processReferenceConfig(nextSibling);
}

void ProjectReference_Private::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("Name"))
                m_name = domElement.value();

            else if (domElement.name() == QLatin1String("ReferencedProjectIdentifier"))
                m_referencedProjectIdentifier = domElement.value();
        }
    }
}


ProjectReference2003_Private::~ProjectReference2003_Private()
{
}

ProjectReference_Private::Ptr ProjectReference2003_Private::clone() const
{
    return ProjectReference_Private::Ptr(new ProjectReference2003_Private(*this));
}

ProjectReference2003_Private::ProjectReference2003_Private()
{
}

ProjectReference2003_Private::ProjectReference2003_Private(const ProjectReference2003_Private &projRef_p)
    : ProjectReference_Private(projRef_p)
{
}

ProjectReference2003_Private &ProjectReference2003_Private::operator =(const ProjectReference2003_Private &projRef_p)
{
    ProjectReference_Private::operator =(projRef_p);
    return *this;
}

Configuration::Ptr ProjectReference2003_Private::createReferenceConfiguration() const
{
    return ConfigurationsFactory::createConfiguration(VcDocConstants::DV_MSVC_2003, QLatin1String("ReferenceConfiguration"));
}


ProjectReference2005_Private::~ProjectReference2005_Private()
{
}

QDomNode ProjectReference2005_Private::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement toolNode = ProjectReference2003_Private::toXMLDomNode(domXMLDocument).toElement();

    toolNode.setAttribute(QLatin1String("CopyLocal"), m_copyLocal);
    toolNode.setAttribute(QLatin1String("UseInBuild"), QVariant(m_useInBuild).toString());
    toolNode.setAttribute(QLatin1String("RelativePathFromSolution"), m_relativePathFromSolution);

    return toolNode;
}

ProjectReference_Private::Ptr ProjectReference2005_Private::clone() const
{
    return ProjectReference_Private::Ptr(new ProjectReference2005_Private(*this));
}

QString ProjectReference2005_Private::copyLocal() const
{
    return m_copyLocal;
}

void ProjectReference2005_Private::setCopyLocal(const QString &copyLocal)
{
    m_copyLocal = copyLocal;
}

bool ProjectReference2005_Private::useInBuild() const
{
    return m_useInBuild;
}

void ProjectReference2005_Private::setUseInBuild(bool useInBuild)
{
    m_useInBuild = useInBuild;
}

QString ProjectReference2005_Private::relativePathFromSolution() const
{
    return m_relativePathFromSolution;
}

void ProjectReference2005_Private::setRelativePathFromSolution(const QString &relativePathFromSolution)
{
    m_relativePathFromSolution = relativePathFromSolution;
}

ProjectReference2005_Private::ProjectReference2005_Private()
{
}

ProjectReference2005_Private::ProjectReference2005_Private(const ProjectReference2005_Private &projRef_p)
    : ProjectReference2003_Private(projRef_p)
{
}

ProjectReference2005_Private &ProjectReference2005_Private::operator =(const ProjectReference2005_Private &projRef_p)
{
    ProjectReference2003_Private::operator =(projRef_p);
    return *this;
}

void ProjectReference2005_Private::processNodeAttributes(const QDomElement &element)
{
    ProjectReference2003_Private::processNodeAttributes(element);

    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("CopyLocal"))
                m_copyLocal = domElement.value();

            else if (domElement.name() == QLatin1String("RelativePathFromSolution"))
                m_relativePathFromSolution = domElement.value();

            else if (domElement.name() == QLatin1String("UseInBuild")) {
                if (domElement.value() == QLatin1String("false"))
                    m_useInBuild = false;
                else
                    m_useInBuild = true;
            }
        }
    }
}

Configuration::Ptr ProjectReference2005_Private::createReferenceConfiguration() const
{
    return ConfigurationsFactory::createConfiguration(VcDocConstants::DV_MSVC_2005, QLatin1String("ReferenceConfiguration"));
}


ProjectReference2008_Private::~ProjectReference2008_Private()
{
}

QDomNode ProjectReference2008_Private::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement toolNode = ProjectReference2005_Private::toXMLDomNode(domXMLDocument).toElement();

    toolNode.setAttribute(QLatin1String("RelativePathToProject"), m_relativePathToProject);
    toolNode.setAttribute(QLatin1String("UseDependenciesInBuild"), QVariant(m_useDependenciesInBuild).toString());
    toolNode.setAttribute(QLatin1String("CopyLocalSatelliteAssemblies"), QVariant(m_copyLocalSatelliteAssemblies).toString());
    toolNode.setAttribute(QLatin1String("CopyLocalDependencies"), QVariant(m_copyLocalDependencies).toString());

    return toolNode;
}

ProjectReference_Private::Ptr ProjectReference2008_Private::clone() const
{
    return ProjectReference_Private::Ptr(new ProjectReference2008_Private(*this));
}

QString ProjectReference2008_Private::relativePathToProject() const
{
    return m_relativePathToProject;
}

void ProjectReference2008_Private::setRelativePathToProject(const QString &relativePathToProject)
{
    m_relativePathToProject = relativePathToProject;
}

bool ProjectReference2008_Private::useDependenciesInBuild() const
{
    return m_useDependenciesInBuild;
}

void ProjectReference2008_Private::setUseDependenciesInBuild(bool useDependenciesInBuild)
{
    m_useDependenciesInBuild = useDependenciesInBuild;
}

bool ProjectReference2008_Private::copyLocalSatelliteAssemblies() const
{
    return m_copyLocalSatelliteAssemblies;
}

void ProjectReference2008_Private::setCopyLocalSatelliteAssemblies(bool copyLocalSatelliteAssemblies)
{
    m_copyLocalSatelliteAssemblies = copyLocalSatelliteAssemblies;
}

bool ProjectReference2008_Private::copyLocalDependencies() const
{
    return m_copyLocalDependencies;
}

void ProjectReference2008_Private::setCopyLocalDependencies(bool copyLocalDependencies)
{
    m_copyLocalDependencies = copyLocalDependencies;
}

ProjectReference2008_Private::ProjectReference2008_Private()
{
}

ProjectReference2008_Private::ProjectReference2008_Private(const ProjectReference2008_Private &projRef_p)
    : ProjectReference2005_Private(projRef_p)
{
}

ProjectReference2008_Private &ProjectReference2008_Private::operator =(const ProjectReference2008_Private &projRef_p)
{
    ProjectReference2005_Private::operator =(projRef_p);
    return *this;
}

void ProjectReference2008_Private::processNodeAttributes(const QDomElement &element)
{
    ProjectReference2005_Private::processNodeAttributes(element);

    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("CopyLocalDependencies")) {
                if (domElement.value() == QLatin1String("false"))
                    m_copyLocalDependencies = false;
                else
                    m_copyLocalDependencies = true;
            }

            else if (domElement.name() == QLatin1String("CopyLocalSatelliteAssemblies")) {
                if (domElement.value() == QLatin1String("false"))
                    m_copyLocalSatelliteAssemblies = false;
                else
                    m_copyLocalSatelliteAssemblies = true;
            }

            else if (domElement.name() == QLatin1String("UseDependenciesInBuild")) {
                if (domElement.value() == QLatin1String("false"))
                    m_useDependenciesInBuild = false;
                else
                    m_useDependenciesInBuild = true;
            }

            else if (domElement.name() == QLatin1String("RelativePathToProject"))
                m_relativePathToProject = domElement.value();
        }
    }
}

Configuration::Ptr ProjectReference2008_Private::createReferenceConfiguration() const
{
    return ConfigurationsFactory::createConfiguration(VcDocConstants::DV_MSVC_2008, QLatin1String("ReferenceConfiguration"));
}

} // namespace Internal
} // namespace VcProjectManager
