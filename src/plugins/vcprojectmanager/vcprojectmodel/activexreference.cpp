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
#include "activexreference.h"

#include <QVariant>
#include "configurationsfactory.h"

namespace VcProjectManager {
namespace Internal {

ActiveXReference::~ActiveXReference()
{
    qDeleteAll(m_referenceConfigurations);
}

void ActiveXReference::processNode(const QDomNode &node)
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

void ActiveXReference::processNodeAttributes(const QDomElement &element)
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

VcNodeWidget *ActiveXReference::createSettingsWidget()
{
    // TODO(Radovan): Finish implementation
    return 0;
}

QDomNode ActiveXReference::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement activeXNode = domXMLDocument.createElement(QLatin1String("ActiveXReference"));

    activeXNode.setAttribute(QLatin1String("ControlGUID"), m_controlGUID);
    activeXNode.setAttribute(QLatin1String("ControlVersion"), m_controlVersion);
    activeXNode.setAttribute(QLatin1String("WrapperTool"), m_wrapperTool);

    foreach (const IConfiguration *refConfig, m_referenceConfigurations)
        activeXNode.appendChild(refConfig->toXMLDomNode(domXMLDocument));

    return activeXNode;
}

void ActiveXReference::addReferenceConfiguration(IConfiguration *refConfig)
{
    if (m_referenceConfigurations.contains(refConfig))
        return;

    // Don't add configuration with the same name
    foreach (const IConfiguration *refConf, m_referenceConfigurations) {
        if (refConfig->fullName() == refConf->fullName())
            return;
    }

    m_referenceConfigurations.append(refConfig);
}

void ActiveXReference::removeReferenceConfiguration(IConfiguration *refConfig)
{
    m_referenceConfigurations.removeAll(refConfig);
}

void ActiveXReference::removeReferenceConfiguration(const QString &refConfigName)
{
    foreach (IConfiguration *refConfig, m_referenceConfigurations) {
        if (refConfig->fullName() == refConfigName) {
            m_referenceConfigurations.removeOne(refConfig);
            delete refConfig;
            return;
        }
    }
}

IConfiguration* ActiveXReference::referenceConfiguration(const QString &refConfigName) const
{
    foreach (IConfiguration *refConfig, m_referenceConfigurations) {
        if (refConfig->fullName() == refConfigName)
            return refConfig;
    }

    return 0;
}

QString ActiveXReference::controlGUID() const
{
    return m_controlGUID;
}

void ActiveXReference::setControlGUID(const QString &ctrlGUID)
{
    m_controlGUID = ctrlGUID;
}

QString ActiveXReference::controlVersion() const
{
    return m_controlVersion;
}

void ActiveXReference::setControlVersion(const QString &ctrlVersion)
{
    m_controlVersion = ctrlVersion;
}

QString ActiveXReference::wrapperTool() const
{
    return m_wrapperTool;
}

void ActiveXReference::setWrapperTool(const QString &wrapperTool)
{
    m_wrapperTool = wrapperTool;
}

ActiveXReference::ActiveXReference()
{
}

ActiveXReference::ActiveXReference(const ActiveXReference &ref)
{
    m_controlGUID = ref.m_controlGUID;
    m_controlVersion = ref.m_controlVersion;
    m_wrapperTool = ref.m_wrapperTool;

    foreach (const IConfiguration *refConf, ref.m_referenceConfigurations)
        m_referenceConfigurations.append(refConf->clone());
}

ActiveXReference &ActiveXReference::operator =(const ActiveXReference &ref)
{
    if (this != &ref) {
        m_controlGUID = ref.m_controlGUID;
        m_controlVersion = ref.m_controlVersion;
        m_wrapperTool = ref.m_wrapperTool;

        m_referenceConfigurations.clear();

        foreach (const IConfiguration *refConf, ref.m_referenceConfigurations)
            m_referenceConfigurations.append(refConf->clone());
    }

    return *this;
}

void ActiveXReference::processReferenceConfig(const QDomNode &referenceConfig)
{
    IConfiguration *referenceConfiguration = createReferenceConfiguration();
    referenceConfiguration->processNode(referenceConfig);
    m_referenceConfigurations.append(referenceConfiguration);

    // process next sibling
    QDomNode nextSibling = referenceConfig.nextSibling();
    if (!nextSibling.isNull())
        processReferenceConfig(nextSibling);
}

ActiveXReference2003::ActiveXReference2003(const ActiveXReference2003 &ref)
    : ActiveXReference(ref)
{
}

ActiveXReference2003 &ActiveXReference2003::operator =(const ActiveXReference2003 &ref)
{
    ActiveXReference::operator =(ref);
    return *this;
}

ActiveXReference2003::~ActiveXReference2003()
{
}

ActiveXReference::Ptr ActiveXReference2003::clone() const
{
    return ActiveXReference::Ptr(new ActiveXReference2003(*this));
}

ActiveXReference2003::ActiveXReference2003()
{
}

IConfiguration* ActiveXReference2003::createReferenceConfiguration() const
{
    return new Configuration2003(QLatin1String("ReferenceConfiguration"));
}


ActiveXReference2005::ActiveXReference2005(const ActiveXReference2005 &ref)
    : ActiveXReference2003(ref)
{
}

ActiveXReference2005 &ActiveXReference2005::operator =(const ActiveXReference2005 &ref)
{
    ActiveXReference2003::operator =(ref);
    return *this;
}

ActiveXReference2005::~ActiveXReference2005()
{
}

void ActiveXReference2005::processNodeAttributes(const QDomElement &element)
{
    ActiveXReference2003::processNodeAttributes(element);

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

QDomNode ActiveXReference2005::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement activeXNode = ActiveXReference2003::toXMLDomNode(domXMLDocument).toElement();

    activeXNode.setAttribute(QLatin1String("LocaleID"), m_localeID);
    activeXNode.setAttribute(QLatin1String("CopyLocal"), m_copyLocal);
    activeXNode.setAttribute(QLatin1String("UseInBuild"), QVariant(m_useInBuild).toString());

    return activeXNode;
}

ActiveXReference::Ptr ActiveXReference2005::clone() const
{
    return ActiveXReference::Ptr(new ActiveXReference2005(*this));
}

QString ActiveXReference2005::localeID() const
{
    return m_localeID;
}

void ActiveXReference2005::setLocaleID(const QString &localeID)
{
    m_localeID = localeID;
}

QString ActiveXReference2005::copyLocal() const
{
    return m_copyLocal;
}

void ActiveXReference2005::setCopyLocal(const QString &copyLocal)
{
    m_copyLocal = copyLocal;
}

bool ActiveXReference2005::useInBuild() const
{
    return m_useInBuild;
}

void ActiveXReference2005::setUseInBuild(bool useInBuild)
{
    m_useInBuild = useInBuild;
}

ActiveXReference2005::ActiveXReference2005()
{
}

IConfiguration* ActiveXReference2005::createReferenceConfiguration() const
{
    return new Configuration2005(QLatin1String("ReferenceConfiguration"));
}


ActiveXReference2008::ActiveXReference2008(const ActiveXReference2008 &ref)
    : ActiveXReference2005(ref)
{
}

ActiveXReference2008 &ActiveXReference2008::operator =(const ActiveXReference2008 &ref)
{
    ActiveXReference2005::operator =(ref);
    return *this;
}

ActiveXReference2008::~ActiveXReference2008()
{
}

void ActiveXReference2008::processNodeAttributes(const QDomElement &element)
{
    ActiveXReference2005::processNodeAttributes(element);

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

QDomNode ActiveXReference2008::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement activeXNode = ActiveXReference2005::toXMLDomNode(domXMLDocument).toElement();

    activeXNode.setAttribute(QLatin1String("CopyLocalDependencies"), QVariant(m_copyLocalDependencies).toString());
    activeXNode.setAttribute(QLatin1String("CopyLocalSatelliteAssemblies"), QVariant(m_copyLocalSatelliteAssemblies).toString());
    activeXNode.setAttribute(QLatin1String("UseDependenciesInBuild"), QVariant(m_useDependenciesInBuild).toString());

    return activeXNode;
}

ActiveXReference::Ptr ActiveXReference2008::clone() const
{
    return ActiveXReference::Ptr(new ActiveXReference2005(*this));
}

bool ActiveXReference2008::copyLocalDependencies() const
{
    return m_copyLocalDependencies;
}

void ActiveXReference2008::setCopyLocalDependencies(bool copyLocalDependencies)
{
    m_copyLocalDependencies = copyLocalDependencies;
}

bool ActiveXReference2008::copyLocalSatelliteAssemblies() const
{
    return m_copyLocalSatelliteAssemblies;
}

void ActiveXReference2008::setCopyLocalSatelliteAssemblies(bool copyLocalSatelliteAssemblies)
{
    m_copyLocalSatelliteAssemblies = copyLocalSatelliteAssemblies;
}

bool ActiveXReference2008::useDependenciesInBuild() const
{
    return m_useDependenciesInBuild;
}

void ActiveXReference2008::setUseDependenciesInBuild(bool useDependenciesInBuild)
{
    m_useDependenciesInBuild = useDependenciesInBuild;
}

ActiveXReference2008::ActiveXReference2008()
{
}

IConfiguration* ActiveXReference2008::createReferenceConfiguration() const
{
    return new Configuration2008(QLatin1String("ReferenceConfiguration"));
}


ActiveXReferenceFactory &ActiveXReferenceFactory::instance()
{
    static ActiveXReferenceFactory am;
    return am;
}

ActiveXReference::Ptr ActiveXReferenceFactory::create(VcDocConstants::DocumentVersion version)
{
    ActiveXReference::Ptr ref;

    switch (version) {
    case VcDocConstants::DV_MSVC_2003:
        ref = ActiveXReference::Ptr(new ActiveXReference2003);
        break;
    case VcDocConstants::DV_MSVC_2005:
        ref = ActiveXReference::Ptr(new ActiveXReference2005);
        break;
    case VcDocConstants::DV_MSVC_2008:
        ref = ActiveXReference::Ptr(new ActiveXReference2008);
        break;
    }

    return ref;
}

} // namespace Internal
} // namespace VcProjectManager
