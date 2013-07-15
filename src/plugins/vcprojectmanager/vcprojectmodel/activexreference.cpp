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
#include "activexreference.h"

#include "referenceconfiguration.h"

namespace VcProjectManager {
namespace Internal {

ActiveXReference::~ActiveXReference()
{
}

void ActiveXReference::processNode(const QDomNode &node)
{
    m_private->processNode(node);
}

void ActiveXReference::processNodeAttributes(const QDomElement &element)
{
    Q_UNUSED(element)
}

VcNodeWidget *ActiveXReference::createSettingsWidget()
{
    // TODO(Radovan): Finish implementation
    return 0;
}

QDomNode ActiveXReference::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    return m_private->toXMLDomNode(domXMLDocument);
}

void ActiveXReference::addReferenceConfiguration(ReferenceConfiguration::Ptr refConfig)
{
    m_private->addReferenceConfiguration(refConfig);
}

void ActiveXReference::removeReferenceConfiguration(ReferenceConfiguration::Ptr refConfig)
{
    m_private->removeReferenceConfiguration(refConfig);
}

void ActiveXReference::removeReferenceConfiguration(const QString &refConfigName)
{
    m_private->removeReferenceConfiguration(refConfigName);
}

QList<ReferenceConfiguration::Ptr> ActiveXReference::referenceConfigurations() const
{
    return m_private->referenceConfigurations();
}

ReferenceConfiguration::Ptr ActiveXReference::referenceConfiguration(const QString &refConfigName) const
{
    return m_private->referenceConfiguration(refConfigName);
}

QString ActiveXReference::controlGUID() const
{
    return m_private->controlGUID();
}

void ActiveXReference::setControlGUID(const QString &ctrlGUID)
{
    m_private->setControlGUID(ctrlGUID);
}

QString ActiveXReference::controlVersion() const
{
    return m_private->controlVersion();
}

void ActiveXReference::setControlVersion(const QString &ctrlVersion)
{
    m_private->setControlVersion(ctrlVersion);
}

QString ActiveXReference::wrapperTool() const
{
    return m_private->wrapperTool();
}

void ActiveXReference::setWrapperTool(const QString &wrapperTool)
{
    m_private->setWrapperTool(wrapperTool);
}

ActiveXReference::ActiveXReference()
{
}

ActiveXReference::ActiveXReference(const ActiveXReference &ref)
{
    m_private = ref.m_private->clone();
}

ActiveXReference &ActiveXReference::operator =(const ActiveXReference &ref)
{
    if (this != &ref)
        m_private = ref.m_private->clone();
    return *this;
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

void ActiveXReference2003::init()
{
    m_private = ActiveXReference_Private::Ptr(new ActiveXReference2003_Private);
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

ActiveXReference::Ptr ActiveXReference2005::clone() const
{
    return ActiveXReference::Ptr(new ActiveXReference2005(*this));
}

QString ActiveXReference2005::localeID() const
{
    QSharedPointer<ActiveXReference2005_Private> activeRef = m_private.staticCast<ActiveXReference2005_Private>();
    return activeRef->localeID();
}

void ActiveXReference2005::setLocaleID(const QString &localeID)
{
    QSharedPointer<ActiveXReference2005_Private> activeRef = m_private.staticCast<ActiveXReference2005_Private>();
    activeRef->setLocaleID(localeID);
}

QString ActiveXReference2005::copyLocal() const
{
    QSharedPointer<ActiveXReference2005_Private> activeRef = m_private.staticCast<ActiveXReference2005_Private>();
    return activeRef->copyLocal();
}

void ActiveXReference2005::setCopyLocal(const QString &copyLocal)
{
    QSharedPointer<ActiveXReference2005_Private> activeRef = m_private.staticCast<ActiveXReference2005_Private>();
    activeRef->setCopyLocal(copyLocal);
}

bool ActiveXReference2005::useInBuild() const
{
    QSharedPointer<ActiveXReference2005_Private> activeRef = m_private.staticCast<ActiveXReference2005_Private>();
    return activeRef->useInBuild();
}

void ActiveXReference2005::setUseInBuild(bool useInBuild)
{
    QSharedPointer<ActiveXReference2005_Private> activeRef = m_private.staticCast<ActiveXReference2005_Private>();
    activeRef->setUseInBuild(useInBuild);
}

ActiveXReference2005::ActiveXReference2005()
{
}

void ActiveXReference2005::init()
{
    m_private = ActiveXReference_Private::Ptr(new ActiveXReference2005_Private);
}


ActiveXReference2008::ActiveXReference2008()
{
}

void ActiveXReference2008::init()
{
    m_private = ActiveXReference_Private::Ptr(new ActiveXReference2008_Private);
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

ActiveXReference::Ptr ActiveXReference2008::clone() const
{
    return ActiveXReference::Ptr(new ActiveXReference2005(*this));
}

bool ActiveXReference2008::copyLocalDependencies() const
{
    QSharedPointer<ActiveXReference2008_Private> activeRef = m_private.staticCast<ActiveXReference2008_Private>();
    return activeRef->copyLocalDependencies();
}

void ActiveXReference2008::setCopyLocalDependencies(bool copyLocalDependencies)
{
    QSharedPointer<ActiveXReference2008_Private> activeRef = m_private.staticCast<ActiveXReference2008_Private>();
    activeRef->setCopyLocalDependencies(copyLocalDependencies);
}

bool ActiveXReference2008::copyLocalSatelliteAssemblies() const
{
    QSharedPointer<ActiveXReference2008_Private> activeRef = m_private.staticCast<ActiveXReference2008_Private>();
    return activeRef->copyLocalSatelliteAssemblies();
}

void ActiveXReference2008::setCopyLocalSatelliteAssemblies(bool copyLocalSatelliteAssemblies)
{
    QSharedPointer<ActiveXReference2008_Private> activeRef = m_private.staticCast<ActiveXReference2008_Private>();
    activeRef->setCopyLocalSatelliteAssemblies(copyLocalSatelliteAssemblies);
}

bool ActiveXReference2008::useDependenciesInBuild() const
{
    QSharedPointer<ActiveXReference2008_Private> activeRef = m_private.staticCast<ActiveXReference2008_Private>();
    return activeRef->useDependenciesInBuild();
}

void ActiveXReference2008::setUseDependenciesInBuild(bool useDependenciesInBuild)
{
    QSharedPointer<ActiveXReference2008_Private> activeRef = m_private.staticCast<ActiveXReference2008_Private>();
    activeRef->setUseDependenciesInBuild(useDependenciesInBuild);
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

    if (ref)
        ref->init();

    return ref;
}

} // namespace Internal
} // namespace VcProjectManager
