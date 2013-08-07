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
#include "assemblyreference.h"

#include <QVariant>

#include "configurationsfactory.h"

namespace VcProjectManager {
namespace Internal {

AssemblyReference::~AssemblyReference()
{
}

void AssemblyReference::processNode(const QDomNode &node)
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

void AssemblyReference::processNodeAttributes(const QDomElement &element)
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

VcNodeWidget *AssemblyReference::createSettingsWidget()
{
    return 0;
}

QDomNode AssemblyReference::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement assemblyRefNode = domXMLDocument.createElement(QLatin1String("AssemblyReference"));

    assemblyRefNode.setAttribute(QLatin1String("RelativePath"), m_relativePath);

    foreach (const Configuration::Ptr &refConfig, m_referenceConfigurations)
        assemblyRefNode.appendChild(refConfig->toXMLDomNode(domXMLDocument));

    return assemblyRefNode;
}

QString AssemblyReference::relativePath() const
{
    return m_relativePath;
}

void AssemblyReference::setRelativePath(const QString &relativePath)
{
    m_relativePath = relativePath;
}

void AssemblyReference::addReferenceConfiguration(Configuration::Ptr refConfig)
{
    if (m_referenceConfigurations.contains(refConfig))
        return;

    foreach (const Configuration::Ptr &refConf, m_referenceConfigurations) {
        if (refConfig->name() == refConf->name())
            return;
    }

    m_referenceConfigurations.append(refConfig);
}

void AssemblyReference::removeReferenceConfiguration(Configuration::Ptr refConfig)
{
    m_referenceConfigurations.removeAll(refConfig);
}

void AssemblyReference::removeReferenceConfiguration(const QString &refConfName)
{
    foreach (const Configuration::Ptr &refConfig, m_referenceConfigurations) {
        if (refConfig->name() == refConfName) {
            removeReferenceConfiguration(refConfig);
            return;
        }
    }
}

QList<Configuration::Ptr> AssemblyReference::referenceConfigurations() const
{
    return m_referenceConfigurations;
}

Configuration::Ptr AssemblyReference::referenceConfiguration(const QString &refConfigName) const
{
    foreach (const Configuration::Ptr &refConfig, m_referenceConfigurations) {
        if (refConfig->name() == refConfigName)
            return refConfig;
    }
    return Configuration::Ptr();
}

AssemblyReference::AssemblyReference()
{
}

AssemblyReference::AssemblyReference(const AssemblyReference &asmRef)
{
    m_relativePath = asmRef.m_relativePath;

    foreach (const Configuration::Ptr &refConfig, asmRef.m_referenceConfigurations)
        m_referenceConfigurations.append(refConfig->clone());
}

AssemblyReference &AssemblyReference::operator =(const AssemblyReference &asmRef)
{
    if (this != &asmRef) {
        m_relativePath = asmRef.m_relativePath;

        foreach (const Configuration::Ptr &refConfig, asmRef.m_referenceConfigurations)
            m_referenceConfigurations.append(refConfig->clone());
    }

    return *this;
}

void AssemblyReference::processReferenceConfig(const QDomNode &referenceConfig)
{
    Configuration::Ptr referenceConfiguration = createReferenceConfiguration();
    referenceConfiguration->processNode(referenceConfig);
    m_referenceConfigurations.append(referenceConfiguration);

    // process next sibling
    QDomNode nextSibling = referenceConfig.nextSibling();
    if (!nextSibling.isNull())
        processReferenceConfig(nextSibling);
}


AssemblyReference2003::AssemblyReference2003(const AssemblyReference2003 &ref)
    : AssemblyReference(ref)
{
}

AssemblyReference2003 &AssemblyReference2003::operator =(const AssemblyReference2003 &ref)
{
    AssemblyReference::operator =(ref);
    return *this;
}

AssemblyReference2003::~AssemblyReference2003()
{
}

AssemblyReference::Ptr AssemblyReference2003::clone() const
{
    return AssemblyReference::Ptr(new AssemblyReference2003(*this));
}

AssemblyReference2003::AssemblyReference2003()
{
}

Configuration::Ptr AssemblyReference2003::createReferenceConfiguration() const
{
    return ConfigurationsFactory::createConfiguration(VcDocConstants::DV_MSVC_2003, QLatin1String("ReferenceConfiguration"));
}


AssemblyReference2005::AssemblyReference2005(const AssemblyReference2005 &ref)
    : AssemblyReference2003(ref)
{
    m_assemblyName = ref.m_assemblyName;
    m_copyLocal = ref.m_copyLocal;
    m_useInBuild = ref.m_useInBuild;
}

AssemblyReference2005 &AssemblyReference2005::operator =(const AssemblyReference2005 &ref)
{
    if (this != &ref) {
        AssemblyReference2003::operator =(ref);
        m_assemblyName = ref.m_assemblyName;
        m_copyLocal = ref.m_copyLocal;
        m_useInBuild = ref.m_useInBuild;
    }

    return *this;
}

AssemblyReference2005::~AssemblyReference2005()
{
}

QDomNode AssemblyReference2005::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement assemblyRefNode = AssemblyReference2003::toXMLDomNode(domXMLDocument).toElement();

    assemblyRefNode.setAttribute(QLatin1String("AssemblyName"), m_assemblyName);
    assemblyRefNode.setAttribute(QLatin1String("CopyLocal"), QVariant(m_copyLocal).toString());
    assemblyRefNode.setAttribute(QLatin1String("UseInBuild"), QVariant(m_useInBuild).toString());

    return assemblyRefNode;
}

AssemblyReference::Ptr AssemblyReference2005::clone() const
{
    return AssemblyReference::Ptr(new AssemblyReference2005(*this));
}

QString AssemblyReference2005::assemblyName() const
{
    return m_assemblyName;
}

void AssemblyReference2005::setAssemblyName(const QString &assemblyName)
{
    m_assemblyName = assemblyName;
}

bool AssemblyReference2005::copyLocal() const
{
    return m_copyLocal;
}

void AssemblyReference2005::setCopyLocal(bool copyLocal)
{
    m_copyLocal = copyLocal;
}

bool AssemblyReference2005::useInBuild() const
{
    return m_useInBuild;
}

void AssemblyReference2005::setUseInBuild(bool useInBuild)
{
    m_useInBuild = useInBuild;
}

AssemblyReference2005::AssemblyReference2005()
{
}

Configuration::Ptr AssemblyReference2005::createReferenceConfiguration() const
{
    return ConfigurationsFactory::createConfiguration(VcDocConstants::DV_MSVC_2005, QLatin1String("ReferenceConfiguration"));
}

void AssemblyReference2005::processNodeAttributes(const QDomElement &element)
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


AssemblyReference2008::AssemblyReference2008(const AssemblyReference2008 &ref)
    : AssemblyReference2005(ref)
{
    m_copyLocalDependencies = ref.m_copyLocalDependencies;
    m_copyLocalSatelliteAssemblies = ref.m_copyLocalSatelliteAssemblies;
    m_useDependenciesInBuild = ref.m_useDependenciesInBuild;
    m_subType = ref.m_subType;
    m_minFrameworkVersion = ref.m_minFrameworkVersion;
}

AssemblyReference2008 &AssemblyReference2008::operator =(const AssemblyReference2008 &ref)
{
    if (this != &ref) {
        AssemblyReference2005::operator =(ref);
        m_copyLocalDependencies = ref.m_copyLocalDependencies;
        m_copyLocalSatelliteAssemblies = ref.m_copyLocalSatelliteAssemblies;
        m_useDependenciesInBuild = ref.m_useDependenciesInBuild;
        m_subType = ref.m_subType;
        m_minFrameworkVersion = ref.m_minFrameworkVersion;
    }

    return *this;
}

AssemblyReference2008::~AssemblyReference2008()
{
}

QDomNode AssemblyReference2008::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement assemblyRefNode = AssemblyReference2005::toXMLDomNode(domXMLDocument).toElement();

    assemblyRefNode.setAttribute(QLatin1String("CopyLocalDependencies"), QVariant(m_copyLocalDependencies).toString());
    assemblyRefNode.setAttribute(QLatin1String("CopyLocalSatelliteAssemblies"), QVariant(m_copyLocalSatelliteAssemblies).toString());
    assemblyRefNode.setAttribute(QLatin1String("UseDependenciesInBuild"), QVariant(m_useDependenciesInBuild).toString());
    assemblyRefNode.setAttribute(QLatin1String("SubType"), m_subType);
    assemblyRefNode.setAttribute(QLatin1String("MinFrameworkVersion"), m_minFrameworkVersion);

    return assemblyRefNode;
}

AssemblyReference::Ptr AssemblyReference2008::clone() const
{
    return AssemblyReference::Ptr(new AssemblyReference2008(*this));
}

bool AssemblyReference2008::copyLocalDependencies() const
{
    return m_copyLocalDependencies;
}

void AssemblyReference2008::setCopyLocalDependencies(bool copyLocalDependencies)
{
    m_copyLocalDependencies = copyLocalDependencies;
}

bool AssemblyReference2008::copyLocalSatelliteAssemblies() const
{
    return m_copyLocalSatelliteAssemblies;
}

void AssemblyReference2008::setCopyLocalSatelliteAssemblies(bool copyLocalSatelliteAssemblies)
{
    m_copyLocalSatelliteAssemblies = copyLocalSatelliteAssemblies;
}

bool AssemblyReference2008::useDependenciesInBuild() const
{
    return m_useDependenciesInBuild;
}

void AssemblyReference2008::setUseDependenciesInBuild(bool useDependenciesInBuild)
{
    m_useDependenciesInBuild = useDependenciesInBuild;
}

QString AssemblyReference2008::subType() const
{
    return m_subType;
}

void AssemblyReference2008::setSubType(const QString &subType)
{
    m_subType = subType;
}

QString AssemblyReference2008::minFrameworkVersion() const
{
    return m_minFrameworkVersion;
}

void AssemblyReference2008::setMinFrameworkVersion(const QString &minFrameworkVersion)
{
    m_minFrameworkVersion = minFrameworkVersion;
}

AssemblyReference2008::AssemblyReference2008()
{
}

Configuration::Ptr AssemblyReference2008::createReferenceConfiguration() const
{
    return ConfigurationsFactory::createConfiguration(VcDocConstants::DV_MSVC_2008, QLatin1String("ReferenceConfiguration"));
}

void AssemblyReference2008::processNodeAttributes(const QDomElement &element)
{
    AssemblyReference2005::processNodeAttributes(element);

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


AssemblyReferenceFactory &AssemblyReferenceFactory::instance()
{
    static AssemblyReferenceFactory as;
    return as;
}

AssemblyReference::Ptr AssemblyReferenceFactory::create(VcDocConstants::DocumentVersion version)
{
    AssemblyReference::Ptr ref;

    switch (version) {
    case VcDocConstants::DV_MSVC_2003:
        ref = AssemblyReference::Ptr(new AssemblyReference2003);
        break;
    case VcDocConstants::DV_MSVC_2005:
        ref = AssemblyReference::Ptr(new AssemblyReference2005);
        break;
    case VcDocConstants::DV_MSVC_2008:
        ref = AssemblyReference::Ptr(new AssemblyReference2008);
        break;
    }

    return ref;
}

} // namespace Internal
} // namespace VcProjectManager
