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
#include "assemblyreference_private.h"

#include <QVariant>

#include "configurationsfactory.h"

namespace VcProjectManager {
namespace Internal {

AssemblyReference_Private::~AssemblyReference_Private()
{
    m_referenceConfigurations.clear();
}

void AssemblyReference_Private::processNode(const QDomNode &node)
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

QDomNode AssemblyReference_Private::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement assemblyRefNode = domXMLDocument.createElement(QLatin1String("AssemblyReference"));

    assemblyRefNode.setAttribute(QLatin1String("RelativePath"), m_relativePath);

    foreach (const Configuration::Ptr &refConfig, m_referenceConfigurations)
        assemblyRefNode.appendChild(refConfig->toXMLDomNode(domXMLDocument));

    return assemblyRefNode;
}

QString AssemblyReference_Private::relativePath() const
{
    return m_relativePath;
}

void AssemblyReference_Private::setRelativePath(const QString &relativePath)
{
    m_relativePath = relativePath;
}

void AssemblyReference_Private::addReferenceConfiguration(Configuration::Ptr refConfig)
{
    if (m_referenceConfigurations.contains(refConfig))
        return;

    foreach (const Configuration::Ptr &refConf, m_referenceConfigurations) {
        if (refConfig->name() == refConf->name())
            return;
    }

    m_referenceConfigurations.append(refConfig);
}

void AssemblyReference_Private::removeReferenceConfiguration(Configuration::Ptr refConfig)
{
    m_referenceConfigurations.removeAll(refConfig);
}

void AssemblyReference_Private::removeReferenceConfiguration(const QString &refConfName)
{
    foreach (const Configuration::Ptr &refConfig, m_referenceConfigurations) {
        if (refConfig->name() == refConfName) {
            removeReferenceConfiguration(refConfig);
            return;
        }
    }
}

QList<Configuration::Ptr> AssemblyReference_Private::referenceConfigurations() const
{
    return m_referenceConfigurations;
}

Configuration::Ptr AssemblyReference_Private::referenceConfiguration(const QString &refConfigName) const
{
    foreach (const Configuration::Ptr &refConfig, m_referenceConfigurations) {
        if (refConfig->name() == refConfigName)
            return refConfig;
    }
    return Configuration::Ptr();
}

AssemblyReference_Private::AssemblyReference_Private()
{
}

AssemblyReference_Private::AssemblyReference_Private(const AssemblyReference_Private &asmPrivate)
{
    m_relativePath = asmPrivate.m_relativePath;

    foreach (const Configuration::Ptr &refConfig, asmPrivate.m_referenceConfigurations)
        m_referenceConfigurations.append(refConfig->clone());
}

AssemblyReference_Private &AssemblyReference_Private::operator =(const AssemblyReference_Private &asmPrivate)
{
    if (this != &asmPrivate) {
        m_relativePath = asmPrivate.m_relativePath;

        foreach (const Configuration::Ptr &refConfig, asmPrivate.m_referenceConfigurations)
            m_referenceConfigurations.append(refConfig->clone());
    }

    return *this;
}

void AssemblyReference_Private::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    if (namedNodeMap.size() == 1) {
        QDomNode domNode = namedNodeMap.item(0);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("RelativePath"))
                m_relativePath = domElement.value();
        }
    }
}

void AssemblyReference_Private::processReferenceConfig(const QDomNode &referenceConfig)
{
    Configuration::Ptr referenceConfiguration = createReferenceConfiguration();
    referenceConfiguration->processNode(referenceConfig);
    m_referenceConfigurations.append(referenceConfiguration);

    // process next sibling
    QDomNode nextSibling = referenceConfig.nextSibling();
    if (!nextSibling.isNull())
        processReferenceConfig(nextSibling);
}


AssemblyReference2003_Private::~AssemblyReference2003_Private()
{
}

AssemblyReference_Private::Ptr AssemblyReference2003_Private::clone() const
{
    return AssemblyReference_Private::Ptr(new AssemblyReference2003_Private(*this));
}

AssemblyReference2003_Private::AssemblyReference2003_Private()
{
}

AssemblyReference2003_Private::AssemblyReference2003_Private(const AssemblyReference2003_Private &ref)
    : AssemblyReference_Private(ref)
{
}

AssemblyReference2003_Private &AssemblyReference2003_Private::operator =(const AssemblyReference2003_Private &ref)
{
    AssemblyReference_Private::operator =(ref);
    return *this;
}

Configuration::Ptr AssemblyReference2003_Private::createReferenceConfiguration() const
{
    return ConfigurationsFactory::createConfiguration(VcDocConstants::DV_MSVC_2003);
}


AssemblyReference2005_Private::~AssemblyReference2005_Private()
{
}

QDomNode AssemblyReference2005_Private::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement assemblyRefNode = AssemblyReference2003_Private::toXMLDomNode(domXMLDocument).toElement();

    assemblyRefNode.setAttribute(QLatin1String("AssemblyName"), m_assemblyName);
    assemblyRefNode.setAttribute(QLatin1String("CopyLocal"), QVariant(m_copyLocal).toString());
    assemblyRefNode.setAttribute(QLatin1String("UseInBuild"), QVariant(m_useInBuild).toString());

    return assemblyRefNode;
}

AssemblyReference_Private::Ptr AssemblyReference2005_Private::clone() const
{
    return AssemblyReference_Private::Ptr(new AssemblyReference2005_Private(*this));
}

QString AssemblyReference2005_Private::assemblyName() const
{
    return m_assemblyName;
}

void AssemblyReference2005_Private::setAssemblyName(const QString &assemblyName)
{
    m_assemblyName = assemblyName;
}

bool AssemblyReference2005_Private::copyLocal() const
{
    return m_copyLocal;
}

void AssemblyReference2005_Private::setCopyLocal(bool copyLocal)
{
    m_copyLocal = copyLocal;
}

bool AssemblyReference2005_Private::useInBuild() const
{
    return m_useInBuild;
}

void AssemblyReference2005_Private::setUseInBuild(bool useInBuild)
{
    m_useInBuild = useInBuild;
}

AssemblyReference2005_Private::AssemblyReference2005_Private()
{
}

AssemblyReference2005_Private::AssemblyReference2005_Private(const AssemblyReference2005_Private &ref)
    : AssemblyReference2003_Private(ref)
{
    m_assemblyName = ref.m_assemblyName;
    m_copyLocal = ref.m_copyLocal;
    m_useInBuild = ref.m_useInBuild;
}

AssemblyReference2005_Private &AssemblyReference2005_Private::operator =(const AssemblyReference2005_Private &ref)
{
    if (this != &ref) {
        AssemblyReference2003_Private::operator =(ref);
        m_assemblyName = ref.m_assemblyName;
        m_copyLocal = ref.m_copyLocal;
        m_useInBuild = ref.m_useInBuild;
    }

    return *this;
}

void AssemblyReference2005_Private::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("RelativePath"))
                m_relativePath = domElement.value();


            else if (domElement.name() == QLatin1String("AssemblyName"))
                m_assemblyName = domElement.value();

            else if (domElement.name() == QLatin1String("CopyLocal")) {
                if (domElement.value() == QLatin1String("false"))
                    m_copyLocal = false;
                else
                    m_copyLocal = true;
            }

            else if (domElement.name() == QLatin1String("UseInBuild")) {
                if (domElement.value() == QLatin1String("false"))
                    m_useInBuild = false;
                else
                    m_useInBuild = true;
            }
        }
    }
}

Configuration::Ptr AssemblyReference2005_Private::createReferenceConfiguration() const
{
    return ConfigurationsFactory::createConfiguration(VcDocConstants::DV_MSVC_2005);
}


AssemblyReference2008_Private::~AssemblyReference2008_Private()
{
}

QDomNode AssemblyReference2008_Private::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement assemblyRefNode = AssemblyReference2005_Private::toXMLDomNode(domXMLDocument).toElement();

    assemblyRefNode.setAttribute(QLatin1String("CopyLocalDependencies"), QVariant(m_copyLocalDependencies).toString());
    assemblyRefNode.setAttribute(QLatin1String("CopyLocalSatelliteAssemblies"), QVariant(m_copyLocalSatelliteAssemblies).toString());
    assemblyRefNode.setAttribute(QLatin1String("UseDependenciesInBuild"), QVariant(m_useDependenciesInBuild).toString());
    assemblyRefNode.setAttribute(QLatin1String("SubType"), m_subType);
    assemblyRefNode.setAttribute(QLatin1String("MinFrameworkVersion"), m_minFrameworkVersion);

    return assemblyRefNode;
}

AssemblyReference_Private::Ptr AssemblyReference2008_Private::clone() const
{
    return AssemblyReference_Private::Ptr(new AssemblyReference2008_Private(*this));
}

bool AssemblyReference2008_Private::copyLocalDependencies() const
{
    return m_copyLocalDependencies;
}

void AssemblyReference2008_Private::setCopyLocalDependencies(bool copyLocalDependencies)
{
    m_copyLocalDependencies = copyLocalDependencies;
}

bool AssemblyReference2008_Private::copyLocalSatelliteAssemblies() const
{
    return m_copyLocalSatelliteAssemblies;
}

void AssemblyReference2008_Private::setCopyLocalSatelliteAssemblies(bool copyLocalSatelliteAssemblies)
{
    m_copyLocalSatelliteAssemblies = copyLocalSatelliteAssemblies;
}

bool AssemblyReference2008_Private::useDependenciesInBuild() const
{
    return m_useDependenciesInBuild;
}

void AssemblyReference2008_Private::setUseDependenciesInBuild(bool useDependenciesInBuild)
{
    m_useDependenciesInBuild = useDependenciesInBuild;
}

QString AssemblyReference2008_Private::subType() const
{
    return m_subType;
}

void AssemblyReference2008_Private::setSubType(const QString &subType)
{
    m_subType = subType;
}

QString AssemblyReference2008_Private::minFrameworkVersion() const
{
    return m_minFrameworkVersion;
}

void AssemblyReference2008_Private::setMinFrameworkVersion(const QString &minFrameworkVersion)
{
    m_minFrameworkVersion = minFrameworkVersion;
}

AssemblyReference2008_Private::AssemblyReference2008_Private()
{
}

AssemblyReference2008_Private::AssemblyReference2008_Private(const AssemblyReference2008_Private &ref)
    : AssemblyReference2005_Private(ref)
{
    m_copyLocalDependencies = ref.m_copyLocalDependencies;
    m_copyLocalSatelliteAssemblies = ref.m_copyLocalSatelliteAssemblies;
    m_useDependenciesInBuild = ref.m_useDependenciesInBuild;
    m_subType = ref.m_subType;
    m_minFrameworkVersion = ref.m_minFrameworkVersion;
}

AssemblyReference2008_Private &AssemblyReference2008_Private::operator =(const AssemblyReference2008_Private &ref)
{
    if (this != &ref) {
        AssemblyReference2005_Private::operator =(ref);
        m_copyLocalDependencies = ref.m_copyLocalDependencies;
        m_copyLocalSatelliteAssemblies = ref.m_copyLocalSatelliteAssemblies;
        m_useDependenciesInBuild = ref.m_useDependenciesInBuild;
        m_subType = ref.m_subType;
        m_minFrameworkVersion = ref.m_minFrameworkVersion;
    }

    return *this;
}

void AssemblyReference2008_Private::processNodeAttributes(const QDomElement &element)
{
    AssemblyReference2005_Private::processNodeAttributes(element);

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

            else if (domElement.name() == QLatin1String("SubType"))
                m_subType = domElement.value();

            else if (domElement.name() == QLatin1String("MinFrameworkVersion"))
                m_minFrameworkVersion = domElement.value();
        }
    }
}

Configuration::Ptr AssemblyReference2008_Private::createReferenceConfiguration() const
{
    return ConfigurationsFactory::createConfiguration(VcDocConstants::DV_MSVC_2008);
}

} // namespace Internal
} // namespace VcProjectManager
