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
#include "activexreference_private.h"

#include <QVariant>

#include "referenceconfigurationfactory.h"

namespace VcProjectManager {
namespace Internal {

ActiveXReference_Private::~ActiveXReference_Private()
{
    m_referenceConfigurations.clear();
}

void ActiveXReference_Private::processNode(const QDomNode &node)
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

QDomNode ActiveXReference_Private::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement activeXNode = domXMLDocument.createElement(QLatin1String("ActiveXReference"));

    activeXNode.setAttribute(QLatin1String("ControlGUID"), m_controlGUID);
    activeXNode.setAttribute(QLatin1String("ControlVersion"), m_controlVersion);
    activeXNode.setAttribute(QLatin1String("WrapperTool"), m_wrapperTool);

    foreach (const ReferenceConfiguration::Ptr &refConfig, m_referenceConfigurations)
        activeXNode.appendChild(refConfig->toXMLDomNode(domXMLDocument));

    return activeXNode;
}

void ActiveXReference_Private::addReferenceConfiguration(ReferenceConfiguration::Ptr refConfig)
{
    if (m_referenceConfigurations.contains(refConfig))
        return;

    // Don't add configuration with the same name
    foreach (const ReferenceConfiguration::Ptr &refConf, m_referenceConfigurations) {
        if (refConfig->name() == refConf->name())
            return;
    }

    m_referenceConfigurations.append(refConfig);
}

void ActiveXReference_Private::removeReferenceConfiguration(ReferenceConfiguration::Ptr refConfig)
{
    m_referenceConfigurations.removeAll(refConfig);
}

void ActiveXReference_Private::removeReferenceConfiguration(const QString &refConfigName)
{
    foreach (const ReferenceConfiguration::Ptr &refConfig, m_referenceConfigurations) {
        if (refConfig->name() == refConfigName) {
            removeReferenceConfiguration(refConfig);
            return;
        }
    }
}

QList<ReferenceConfiguration::Ptr > ActiveXReference_Private::referenceConfigurations() const
{
    return m_referenceConfigurations;
}

ReferenceConfiguration::Ptr ActiveXReference_Private::referenceConfiguration(const QString &refConfigName) const
{
    foreach (const ReferenceConfiguration::Ptr &refConfig, m_referenceConfigurations) {
        if (refConfig->name() == refConfigName)
            return refConfig;
    }

    return ReferenceConfiguration::Ptr();
}

QString ActiveXReference_Private::controlGUID() const
{
    return m_controlGUID;
}

void ActiveXReference_Private::setControlGUID(const QString &ctrlGUID)
{
    m_controlGUID = ctrlGUID;
}

QString ActiveXReference_Private::controlVersion() const
{
    return m_controlVersion;
}

void ActiveXReference_Private::setControlVersion(const QString &ctrlVersion)
{
    m_controlVersion = ctrlVersion;
}

QString ActiveXReference_Private::wrapperTool() const
{
    return m_wrapperTool;
}

void ActiveXReference_Private::setWrapperTool(const QString &wrapperTool)
{
    m_wrapperTool = wrapperTool;
}

ActiveXReference_Private::ActiveXReference_Private()
{
}

ActiveXReference_Private::ActiveXReference_Private(const ActiveXReference_Private &ref)
{
    m_controlGUID = ref.m_controlGUID;
    m_controlVersion = ref.m_controlVersion;
    m_wrapperTool = ref.m_wrapperTool;

    foreach (const ReferenceConfiguration::Ptr &refConf, ref.m_referenceConfigurations)
        m_referenceConfigurations.append(refConf->clone());
}

ActiveXReference_Private &ActiveXReference_Private::operator =(const ActiveXReference_Private &ref)
{
    if (this != &ref) {
        m_controlGUID = ref.m_controlGUID;
        m_controlVersion = ref.m_controlVersion;
        m_wrapperTool = ref.m_wrapperTool;

        m_referenceConfigurations.clear();

        foreach (const ReferenceConfiguration::Ptr &refConf, ref.m_referenceConfigurations)
            m_referenceConfigurations.append(refConf->clone());
    }

    return *this;
}

void ActiveXReference_Private::processReferenceConfig(const QDomNode &referenceConfig)
{
    ReferenceConfiguration::Ptr referenceConfiguration = createReferenceConfiguration();
    referenceConfiguration->processNode(referenceConfig);
    m_referenceConfigurations.append(referenceConfiguration);

    // process next sibling
    QDomNode nextSibling = referenceConfig.nextSibling();
    if (!nextSibling.isNull())
        processReferenceConfig(nextSibling);
}

void ActiveXReference_Private::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("ControlGUID"))
                m_controlGUID = domElement.value();

            else if (domElement.name() == QLatin1String("ControlVersion"))
                m_controlVersion = domElement.value();

            else if (domElement.name() == QLatin1String("WrapperTool"))
                m_wrapperTool = domElement.value();
        }
    }
}


ActiveXReference2003_Private::~ActiveXReference2003_Private()
{
}

ActiveXReference_Private::Ptr ActiveXReference2003_Private::clone() const
{
    return ActiveXReference_Private::Ptr(new ActiveXReference2003_Private(*this));
}

ActiveXReference2003_Private::ActiveXReference2003_Private()
{
}

ActiveXReference2003_Private::ActiveXReference2003_Private(const ActiveXReference2003_Private &ref)
    : ActiveXReference_Private(ref)
{
}

ActiveXReference2003_Private &ActiveXReference2003_Private::operator =(const ActiveXReference2003_Private &ref)
{
    ActiveXReference_Private::operator =(ref);
    return *this;
}

ReferenceConfiguration::Ptr ActiveXReference2003_Private::createReferenceConfiguration() const
{
    return ReferenceConfigurationFactory::createRefConfiguration(VcDocConstants::DV_MSVC_2003);
}


ActiveXReference2005_Private::~ActiveXReference2005_Private()
{
}

QDomNode ActiveXReference2005_Private::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement activeXNode = ActiveXReference2003_Private::toXMLDomNode(domXMLDocument).toElement();

    activeXNode.setAttribute(QLatin1String("LocaleID"), m_localeID);
    activeXNode.setAttribute(QLatin1String("CopyLocal"), m_copyLocal);
    activeXNode.setAttribute(QLatin1String("UseInBuild"), QVariant(m_useInBuild).toString());

    return activeXNode;
}

ActiveXReference_Private::Ptr ActiveXReference2005_Private::clone() const
{
    return ActiveXReference_Private::Ptr(new ActiveXReference2005_Private(*this));
}

QString ActiveXReference2005_Private::localeID() const
{
    return m_localeID;
}

void ActiveXReference2005_Private::setLocaleID(const QString &localeID)
{
    m_localeID = localeID;
}

QString ActiveXReference2005_Private::copyLocal() const
{
    return m_copyLocal;
}

void ActiveXReference2005_Private::setCopyLocal(const QString &copyLocal)
{
    m_copyLocal = copyLocal;
}

bool ActiveXReference2005_Private::useInBuild() const
{
    return m_useInBuild;
}

void ActiveXReference2005_Private::setUseInBuild(bool useInBuild)
{
    m_useInBuild = useInBuild;
}

ActiveXReference2005_Private::ActiveXReference2005_Private()
{
}

ActiveXReference2005_Private::ActiveXReference2005_Private(const ActiveXReference2005_Private &ref)
    : ActiveXReference2003_Private(ref)
{
    m_copyLocal = ref.m_copyLocal;
    m_localeID = ref.m_localeID;
    m_useInBuild = ref.m_useInBuild;
}

ActiveXReference2005_Private &ActiveXReference2005_Private::operator =(const ActiveXReference2005_Private &ref)
{
    if (this != &ref) {
        ActiveXReference2003_Private::operator =(ref);
        m_copyLocal = ref.m_copyLocal;
        m_localeID = ref.m_localeID;
        m_useInBuild = ref.m_useInBuild;
    }
    return *this;
}

void ActiveXReference2005_Private::processNodeAttributes(const QDomElement &element)
{
    ActiveXReference2003_Private::processNodeAttributes(element);

    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("LocaleID"))
                m_localeID = domElement.value();

            else if (domElement.name() == QLatin1String("CopyLocal"))
                m_copyLocal = domElement.value();

            else if (domElement.name() == QLatin1String("UseInBuild")) {
                if (domElement.value() == QLatin1String("false"))
                    m_useInBuild = false;
                else
                    m_useInBuild = true;
            }
        }
    }
}

ReferenceConfiguration::Ptr ActiveXReference2005_Private::createReferenceConfiguration() const
{
    return ReferenceConfigurationFactory::createRefConfiguration(VcDocConstants::DV_MSVC_2005);
}


ActiveXReference2008_Private::~ActiveXReference2008_Private()
{
}

QDomNode ActiveXReference2008_Private::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement activeXNode = ActiveXReference2005_Private::toXMLDomNode(domXMLDocument).toElement();

    activeXNode.setAttribute(QLatin1String("CopyLocalDependencies"), QVariant(m_copyLocalDependencies).toString());
    activeXNode.setAttribute(QLatin1String("CopyLocalSatelliteAssemblies"), QVariant(m_copyLocalSatelliteAssemblies).toString());
    activeXNode.setAttribute(QLatin1String("UseDependenciesInBuild"), QVariant(m_useDependenciesInBuild).toString());

    return activeXNode;
}

ActiveXReference_Private::Ptr ActiveXReference2008_Private::clone() const
{
    return ActiveXReference_Private::Ptr(new ActiveXReference2008_Private(*this));
}

bool ActiveXReference2008_Private::copyLocalDependencies() const
{
    return m_copyLocalDependencies;
}

void ActiveXReference2008_Private::setCopyLocalDependencies(bool copyLocalDependencies)
{
    m_copyLocalDependencies = copyLocalDependencies;
}

bool ActiveXReference2008_Private::copyLocalSatelliteAssemblies() const
{
    return m_copyLocalSatelliteAssemblies;
}

void ActiveXReference2008_Private::setCopyLocalSatelliteAssemblies(bool copyLocalSatelliteAssemblies)
{
    m_copyLocalSatelliteAssemblies = copyLocalSatelliteAssemblies;
}

bool ActiveXReference2008_Private::useDependenciesInBuild() const
{
    return m_useDependenciesInBuild;
}

void ActiveXReference2008_Private::setUseDependenciesInBuild(bool useDependenciesInBuild)
{
    m_useDependenciesInBuild = useDependenciesInBuild;
}

ActiveXReference2008_Private::ActiveXReference2008_Private()
{
}

ActiveXReference2008_Private::ActiveXReference2008_Private(const ActiveXReference2008_Private &ref)
    : ActiveXReference2005_Private(ref)
{
    m_copyLocalDependencies = ref.m_copyLocalDependencies;
    m_copyLocalSatelliteAssemblies = ref.m_copyLocalSatelliteAssemblies;
    m_useDependenciesInBuild = ref.m_useDependenciesInBuild;
}

ActiveXReference2008_Private &ActiveXReference2008_Private::operator =(const ActiveXReference2008_Private &ref)
{
    if (this != &ref) {
        ActiveXReference2005_Private::operator =(ref);

        m_copyLocalDependencies = ref.m_copyLocalDependencies;
        m_copyLocalSatelliteAssemblies = ref.m_copyLocalSatelliteAssemblies;
        m_useDependenciesInBuild = ref.m_useDependenciesInBuild;
    }

    return *this;
}

void ActiveXReference2008_Private::processNodeAttributes(const QDomElement &element)
{
    ActiveXReference2005_Private::processNodeAttributes(element);

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
        }
    }
}

ReferenceConfiguration::Ptr ActiveXReference2008_Private::createReferenceConfiguration() const
{
    return ReferenceConfigurationFactory::createRefConfiguration(VcDocConstants::DV_MSVC_2008);
}

} // namespace Internal
} // namespace VcProjectManager
