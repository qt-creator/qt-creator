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

#include "assemblyreference_private.h"
#include "referenceconfiguration.h"

namespace VcProjectManager {
namespace Internal {

AssemblyReference::~AssemblyReference()
{
}

void AssemblyReference::processNode(const QDomNode &node)
{
    m_private->processNode(node);
}

void AssemblyReference::processNodeAttributes(const QDomElement &element)
{
    Q_UNUSED(element)
}

VcNodeWidget *AssemblyReference::createSettingsWidget()
{
    return 0;
}

QDomNode AssemblyReference::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    return m_private->toXMLDomNode(domXMLDocument);
}

QString AssemblyReference::relativePath() const
{
    return m_private->relativePath();
}

void AssemblyReference::setRelativePath(const QString &relativePath)
{
    m_private->setRelativePath(relativePath);
}

void AssemblyReference::addReferenceConfiguration(ReferenceConfiguration::Ptr refConfig)
{
    m_private->addReferenceConfiguration(refConfig);
}

void AssemblyReference::removeReferenceConfiguration(ReferenceConfiguration::Ptr refConfig)
{
    m_private->removeReferenceConfiguration(refConfig);
}

void AssemblyReference::removeReferenceConfiguration(const QString &refConfName)
{
    m_private->removeReferenceConfiguration(refConfName);
}

QList<ReferenceConfiguration::Ptr> AssemblyReference::referenceConfigurations() const
{
    return m_private->referenceConfigurations();
}

ReferenceConfiguration::Ptr AssemblyReference::referenceConfiguration(const QString &refConfigName) const
{
    return m_private->referenceConfiguration(refConfigName);
}

AssemblyReference::AssemblyReference()
{
}

AssemblyReference::AssemblyReference(const AssemblyReference &asmRef)
{
    m_private = asmRef.m_private->clone();
}

AssemblyReference &AssemblyReference::operator =(const AssemblyReference &asmRef)
{
    if (this != &asmRef)
        m_private = asmRef.m_private->clone();
    return *this;
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

void AssemblyReference2003::init()
{
    m_private = AssemblyReference_Private::Ptr(new AssemblyReference2003_Private);
}


AssemblyReference2005::AssemblyReference2005(const AssemblyReference2005 &ref)
    : AssemblyReference2003(ref)
{
}

AssemblyReference2005 &AssemblyReference2005::operator =(const AssemblyReference2005 &ref)
{
    AssemblyReference2003::operator =(ref);
    return *this;
}

AssemblyReference2005::~AssemblyReference2005()
{
}

AssemblyReference::Ptr AssemblyReference2005::clone() const
{
    return AssemblyReference::Ptr(new AssemblyReference2005(*this));
}

QString AssemblyReference2005::assemblyName() const
{
    QSharedPointer<AssemblyReference2005_Private> asmRef = m_private.staticCast<AssemblyReference2005_Private>();
    return asmRef->assemblyName();
}

void AssemblyReference2005::setAssemblyName(const QString &assemblyName)
{
    QSharedPointer<AssemblyReference2005_Private> asmRef = m_private.staticCast<AssemblyReference2005_Private>();
    asmRef->setAssemblyName(assemblyName);
}

bool AssemblyReference2005::copyLocal() const
{
    QSharedPointer<AssemblyReference2005_Private> asmRef = m_private.staticCast<AssemblyReference2005_Private>();
    return asmRef->copyLocal();
}

void AssemblyReference2005::setCopyLocal(bool copyLocal)
{
    QSharedPointer<AssemblyReference2005_Private> asmRef = m_private.staticCast<AssemblyReference2005_Private>();
    asmRef->setCopyLocal(copyLocal);
}

bool AssemblyReference2005::useInBuild() const
{
    QSharedPointer<AssemblyReference2005_Private> asmRef = m_private.staticCast<AssemblyReference2005_Private>();
    return asmRef->useInBuild();
}

void AssemblyReference2005::setUseInBuild(bool useInBuild)
{
    QSharedPointer<AssemblyReference2005_Private> asmRef = m_private.staticCast<AssemblyReference2005_Private>();
    asmRef->setUseInBuild(useInBuild);
}

AssemblyReference2005::AssemblyReference2005()
{
}

void AssemblyReference2005::init()
{
    m_private = AssemblyReference_Private::Ptr(new AssemblyReference2005_Private);
}


AssemblyReference2008::AssemblyReference2008(const AssemblyReference2008 &ref)
    : AssemblyReference2005(ref)
{
}

AssemblyReference2008 &AssemblyReference2008::operator =(const AssemblyReference2008 &ref)
{
    AssemblyReference2005::operator =(ref);
    return *this;
}

AssemblyReference2008::~AssemblyReference2008()
{
}

AssemblyReference::Ptr AssemblyReference2008::clone() const
{
    return AssemblyReference::Ptr(new AssemblyReference2008(*this));
}

bool AssemblyReference2008::copyLocalDependencies() const
{
    QSharedPointer<AssemblyReference2008_Private> asmRef = m_private.staticCast<AssemblyReference2008_Private>();
    return asmRef->copyLocalDependencies();
}

void AssemblyReference2008::setCopyLocalDependencies(bool copyLocalDependencies)
{
    QSharedPointer<AssemblyReference2008_Private> asmRef = m_private.staticCast<AssemblyReference2008_Private>();
    asmRef->setCopyLocalDependencies(copyLocalDependencies);
}

bool AssemblyReference2008::copyLocalSatelliteAssemblies() const
{
    QSharedPointer<AssemblyReference2008_Private> asmRef = m_private.staticCast<AssemblyReference2008_Private>();
    return asmRef->copyLocalSatelliteAssemblies();
}

void AssemblyReference2008::setCopyLocalSatelliteAssemblies(bool copyLocalSatelliteAssemblies)
{
    QSharedPointer<AssemblyReference2008_Private> asmRef = m_private.staticCast<AssemblyReference2008_Private>();
    asmRef->setCopyLocalSatelliteAssemblies(copyLocalSatelliteAssemblies);
}

bool AssemblyReference2008::useDependenciesInBuild() const
{
    QSharedPointer<AssemblyReference2008_Private> asmRef = m_private.staticCast<AssemblyReference2008_Private>();
    return asmRef->useDependenciesInBuild();
}

void AssemblyReference2008::setUseDependenciesInBuild(bool useDependenciesInBuild)
{
    QSharedPointer<AssemblyReference2008_Private> asmRef = m_private.staticCast<AssemblyReference2008_Private>();
    asmRef->setUseDependenciesInBuild(useDependenciesInBuild);
}

QString AssemblyReference2008::subType() const
{
    QSharedPointer<AssemblyReference2008_Private> asmRef = m_private.staticCast<AssemblyReference2008_Private>();
    return asmRef->subType();
}

void AssemblyReference2008::setSubType(const QString &subType)
{
    QSharedPointer<AssemblyReference2008_Private> asmRef = m_private.staticCast<AssemblyReference2008_Private>();
    asmRef->setSubType(subType);
}

QString AssemblyReference2008::minFrameworkVersion() const
{
    QSharedPointer<AssemblyReference2008_Private> asmRef = m_private.staticCast<AssemblyReference2008_Private>();
    return asmRef->minFrameworkVersion();
}

void AssemblyReference2008::setMinFrameworkVersion(const QString &minFrameworkVersion)
{
    QSharedPointer<AssemblyReference2008_Private> asmRef = m_private.staticCast<AssemblyReference2008_Private>();
    asmRef->setMinFrameworkVersion(minFrameworkVersion);
}

AssemblyReference2008::AssemblyReference2008()
{
}

void AssemblyReference2008::init()
{
    m_private = AssemblyReference_Private::Ptr(new AssemblyReference2008_Private);
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

    if (ref)
        ref->init();

    return ref;
}

} // namespace Internal
} // namespace VcProjectManager
