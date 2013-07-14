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
#ifndef VCPROJECTMANAGER_INTERNAL_ASSEMBLYREFERENCE_PRIVATE_H
#define VCPROJECTMANAGER_INTERNAL_ASSEMBLYREFERENCE_PRIVATE_H

#include "referenceconfiguration.h"
#include "vcprojectdocument_constants.h"

namespace VcProjectManager {
namespace Internal {

class AssemblyReference_Private
{
    friend class AssemblyReference;

public:
    typedef QSharedPointer<AssemblyReference_Private>   Ptr;

    virtual ~AssemblyReference_Private();
    virtual void processNode(const QDomNode &node);
    virtual QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    QString relativePath() const;
    void setRelativePath(const QString &relativePath);
    void addReferenceConfiguration(ReferenceConfiguration::Ptr refConfig);
    void removeReferenceConfiguration(ReferenceConfiguration::Ptr refConfig);
    void removeReferenceConfiguration(const QString &refConfName);
    QList<ReferenceConfiguration::Ptr > referenceConfigurations() const;
    ReferenceConfiguration::Ptr referenceConfiguration(const QString &refConfigName) const;

protected:
    AssemblyReference_Private();
    AssemblyReference_Private(const AssemblyReference_Private &asmPrivate);
    AssemblyReference_Private& operator=(const AssemblyReference_Private &asmPrivate);

    virtual void processNodeAttributes(const QDomElement &element);
    virtual void processReferenceConfig(const QDomNode &referenceConfig);

    /*!
     * Reimplement this to create a new reference configuration.
     * \return A shared pointer to a newly created reference configuration.
     */
    virtual ReferenceConfiguration::Ptr createReferenceConfiguration() const = 0;

    /*!
     * Implement in order to support creating a clone of a AssemblyReference_Private instance.
     * \return A shared pointer to a newly created clone.
     */
    virtual AssemblyReference_Private::Ptr clone() const = 0;

    QList<ReferenceConfiguration::Ptr > m_referenceConfigurations;
    QString m_relativePath; // required
};

class AssemblyReference2003_Private : public AssemblyReference_Private
{
    friend class AssemblyReference2003;

public:
    ~AssemblyReference2003_Private();
    AssemblyReference_Private::Ptr clone() const;

protected:
    AssemblyReference2003_Private();
    AssemblyReference2003_Private(const AssemblyReference2003_Private &ref);
    AssemblyReference2003_Private& operator=(const AssemblyReference2003_Private &ref);
    ReferenceConfiguration::Ptr createReferenceConfiguration() const;
};

class AssemblyReference2005_Private : public AssemblyReference2003_Private
{
    friend class AssemblyReference2005;

public:
    ~AssemblyReference2005_Private();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;
    AssemblyReference_Private::Ptr clone() const;

    QString assemblyName() const;
    void setAssemblyName(const QString &assemblyName);
    bool copyLocal() const;
    void setCopyLocal(bool copyLocal);
    bool useInBuild() const;
    void setUseInBuild(bool useInBuild);

protected:
    AssemblyReference2005_Private();
    AssemblyReference2005_Private(const AssemblyReference2005_Private &ref);
    AssemblyReference2005_Private& operator=(const AssemblyReference2005_Private &ref);
    void processNodeAttributes(const QDomElement &element);
    ReferenceConfiguration::Ptr createReferenceConfiguration() const;

    QString m_assemblyName; // optional
    bool m_copyLocal;       // optional
    bool m_useInBuild;      // optional default: true
};

class AssemblyReference2008_Private : public AssemblyReference2005_Private
{
    friend class AssemblyReference2008;

public:
    ~AssemblyReference2008_Private();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;
    AssemblyReference_Private::Ptr clone() const;

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
    AssemblyReference2008_Private();
    AssemblyReference2008_Private(const AssemblyReference2008_Private &ref);
    AssemblyReference2008_Private& operator=(const AssemblyReference2008_Private &ref);
    void processNodeAttributes(const QDomElement &element);
    ReferenceConfiguration::Ptr createReferenceConfiguration() const;

private:
    bool m_copyLocalDependencies; //optional    default: true
    bool m_copyLocalSatelliteAssemblies; //optional     default: true
    bool m_useDependenciesInBuild; //optional   default: true
    QString m_subType; //optional
    QString m_minFrameworkVersion;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_ASSEMBLYREFERENCE_PRIVATE_H
