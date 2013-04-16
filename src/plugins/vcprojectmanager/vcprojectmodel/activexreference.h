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
#ifndef VCPROJECTMANAGER_INTERNAL_ACTIVEXREFERENCE_H
#define VCPROJECTMANAGER_INTERNAL_ACTIVEXREFERENCE_H

#include "ivcprojectnodemodel.h"

#include "activexreference_private.h"

namespace VcProjectManager {
namespace Internal {

class ActiveXReference : public IVcProjectXMLNode
{
    friend class ActiveXReferenceFactory;

public:
    typedef QSharedPointer<ActiveXReference>    Ptr;

    virtual ~ActiveXReference();
    void processNode(const QDomNode &node);
    void processNodeAttributes(const QDomElement &element);
    VcNodeWidget* createSettingsWidget();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    virtual ActiveXReference::Ptr clone() const = 0;

    void addReferenceConfiguration(ReferenceConfiguration::Ptr refConfig);
    void removeReferenceConfiguration(ReferenceConfiguration::Ptr refConfig);
    void removeReferenceConfiguration(const QString &refConfigName);
    QList<ReferenceConfiguration::Ptr > referenceConfigurations() const;
    ReferenceConfiguration::Ptr referenceConfiguration(const QString &refConfigName) const;

    QString controlGUID() const;
    void setControlGUID(const QString &ctrlGUID);
    QString controlVersion() const;
    void setControlVersion(const QString &ctrlVersion);
    QString wrapperTool() const;
    void setWrapperTool(const QString &wrapperTool);

protected:
    ActiveXReference();
    ActiveXReference(const ActiveXReference &ref);
    ActiveXReference& operator=(const ActiveXReference &ref);
    virtual void init() = 0;

    ActiveXReference_Private::Ptr m_private;
};

class ActiveXReference2003 : public ActiveXReference
{
    friend class ActiveXReferenceFactory;

public:
    ActiveXReference2003(const ActiveXReference2003 &ref);
    ActiveXReference2003& operator=(const ActiveXReference2003 &ref);
    ~ActiveXReference2003();

    ActiveXReference::Ptr clone() const;

protected:
    ActiveXReference2003();
    void init();
};

class ActiveXReference2005 : public ActiveXReference2003
{
    friend class ActiveXReferenceFactory;

public:
    ActiveXReference2005(const ActiveXReference2005 &ref);
    ActiveXReference2005& operator=(const ActiveXReference2005 &ref);
    ~ActiveXReference2005();

    ActiveXReference::Ptr clone() const;

    QString localeID() const;
    void setLocaleID(const QString &localeID);
    QString copyLocal() const;
    void setCopyLocal(const QString &copyLocal);
    bool useInBuild() const;
    void setUseInBuild(bool useInBuild);

protected:
    ActiveXReference2005();
    void init();
};

class ActiveXReference2008 : public ActiveXReference2005
{
    friend class ActiveXReferenceFactory;

public:
    ActiveXReference2008(const ActiveXReference2008 &ref);
    ActiveXReference2008& operator=(const ActiveXReference2008 &ref);
    ~ActiveXReference2008();

    ActiveXReference::Ptr clone() const;

    bool copyLocalDependencies() const;
    void setCopyLocalDependencies(bool copyLocalDependencies);
    bool copyLocalSatelliteAssemblies() const;
    void setCopyLocalSatelliteAssemblies(bool copyLocalSatelliteAssemblies);
    bool useDependenciesInBuild() const;
    void setUseDependenciesInBuild(bool useDependenciesInBuild);

protected:
    ActiveXReference2008();
    void init();
};

class ActiveXReferenceFactory
{
public:
    static ActiveXReferenceFactory& instance();
    ActiveXReference::Ptr create(VcDocConstants::DocumentVersion version);
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_ACTIVEXREFERENCE_H
