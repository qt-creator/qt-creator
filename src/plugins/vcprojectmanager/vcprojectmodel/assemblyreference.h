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
#ifndef VCPROJECTMANAGER_INTERNAL_ASSEMBLYREFERENCE_H
#define VCPROJECTMANAGER_INTERNAL_ASSEMBLYREFERENCE_H

#include "ivcprojectnodemodel.h"

#include "assemblyreference_private.h"

namespace VcProjectManager {
namespace Internal {

class AssemblyReference : public IVcProjectXMLNode
{
    friend class AssemblyReferenceFactory;

public:
    typedef QSharedPointer<AssemblyReference>   Ptr;

    ~AssemblyReference();
    void processNode(const QDomNode &node);
    void processNodeAttributes(const QDomElement &element);
    VcNodeWidget* createSettingsWidget();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    virtual AssemblyReference::Ptr clone() const = 0;

    QString relativePath() const;
    void setRelativePath(const QString &relativePath);
    void addReferenceConfiguration(ReferenceConfiguration::Ptr refConfig);
    void removeReferenceConfiguration(ReferenceConfiguration::Ptr refConfig);
    void removeReferenceConfiguration(const QString &refConfName);
    QList<ReferenceConfiguration::Ptr > referenceConfigurations() const;
    ReferenceConfiguration::Ptr referenceConfiguration(const QString &refConfigName) const;

protected:
    AssemblyReference();
    AssemblyReference(const AssemblyReference &asmRef);
    AssemblyReference& operator=(const AssemblyReference &asmRef);
    virtual void init() = 0;

    AssemblyReference_Private::Ptr m_private;
};

class AssemblyReference2003 : public AssemblyReference
{
    friend class AssemblyReferenceFactory;

public:
    AssemblyReference2003(const AssemblyReference2003 &ref);
    AssemblyReference2003& operator=(const AssemblyReference2003 &ref);
    ~AssemblyReference2003();

    AssemblyReference::Ptr clone() const;

protected:
    AssemblyReference2003();
    void init();
};

class AssemblyReference2005 : public AssemblyReference2003
{
    friend class AssemblyReferenceFactory;

public:
    AssemblyReference2005(const AssemblyReference2005 &ref);
    AssemblyReference2005& operator=(const AssemblyReference2005 &ref);
    ~AssemblyReference2005();

    AssemblyReference::Ptr clone() const;

    QString assemblyName() const;
    void setAssemblyName(const QString &assemblyName);
    bool copyLocal() const;
    void setCopyLocal(bool copyLocal);
    bool useInBuild() const;
    void setUseInBuild(bool useInBuild);

protected:
    AssemblyReference2005();
    void init();
};

class AssemblyReference2008 : public AssemblyReference2005
{
    friend class AssemblyReferenceFactory;

public:
    AssemblyReference2008(const AssemblyReference2008 &ref);
    AssemblyReference2008& operator=(const AssemblyReference2008 &ref);
    ~AssemblyReference2008();

    AssemblyReference::Ptr clone() const;

    bool copyLocalDependencies() const;
    void setCopyLocalDependencies(bool copyLocalDependencies);
    bool copyLocalSatelliteAssemblies() const;
    void setCopyLocalSatelliteAssemblies(bool copyLocalSatelliteAssemblies);
    bool useDependenciesInBuild() const;
    void setUseDependenciesInBuild(bool useDependenciesInBuild);
    QString subType() const;
    void setSubType(const QString &subType);
    QString minFrameworkVersion() const;
    void setMinFrameworkVersion(const QString &minFrameworkVersion);

protected:
    AssemblyReference2008();
    void init();
};

class AssemblyReferenceFactory
{
public:
    static AssemblyReferenceFactory& instance();
    AssemblyReference::Ptr create(VcDocConstants::DocumentVersion version);
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_ASSEMBLYREFERENCE_H
