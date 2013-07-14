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
#ifndef VCPROJECTMANAGER_INTERNAL_ACTIVEXREFERENCE_PRIVATE_H
#define VCPROJECTMANAGER_INTERNAL_ACTIVEXREFERENCE_PRIVATE_H

#include "referenceconfiguration.h"
#include "vcprojectdocument_constants.h"

namespace VcProjectManager {
namespace Internal {

class ActiveXReference_Private
{
    friend class ActiveXReference;

public:
    typedef QSharedPointer<ActiveXReference_Private>    Ptr;

    virtual ~ActiveXReference_Private();
    virtual void processNode(const QDomNode &node);
    virtual QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

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
    ActiveXReference_Private();
    ActiveXReference_Private(const ActiveXReference_Private &ref);
    ActiveXReference_Private& operator=(const ActiveXReference_Private &ref);

    virtual void processReferenceConfig(const QDomNode &referenceConfig);
    virtual void processNodeAttributes(const QDomElement &element);

    /*!
     * Reimplement this to create a new reference configuration.
     * \return A shared pointer to a newly created reference configuration.
     */
    virtual ReferenceConfiguration::Ptr createReferenceConfiguration() const = 0;

    /*!
     * Implement in order to support creating a clone of a ActiveXReference_Private instance.
     * \return A shared pointer to a newly created clone.
     */
    virtual ActiveXReference_Private::Ptr clone() const = 0;

    QString m_controlGUID;  // required
    QString m_controlVersion;   // required
    QString m_wrapperTool;  // required
    QList<ReferenceConfiguration::Ptr > m_referenceConfigurations;
};

class ActiveXReference2003_Private : public ActiveXReference_Private
{
    friend class ActiveXReference2003;
public:
    ~ActiveXReference2003_Private();
    ActiveXReference_Private::Ptr clone() const;

protected:
    ActiveXReference2003_Private();
    ActiveXReference2003_Private(const ActiveXReference2003_Private &ref);
    ActiveXReference2003_Private& operator=(const ActiveXReference2003_Private &ref);
    ReferenceConfiguration::Ptr createReferenceConfiguration() const;
};

class ActiveXReference2005_Private : public ActiveXReference2003_Private
{
    friend class ActiveXReference2005;

public:
    ~ActiveXReference2005_Private();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;
    ActiveXReference_Private::Ptr clone() const;

    QString localeID() const;
    void setLocaleID(const QString &localeID);
    QString copyLocal() const;
    void setCopyLocal(const QString &copyLocal);
    bool useInBuild() const;
    void setUseInBuild(bool useInBuild);

protected:
    ActiveXReference2005_Private();
    ActiveXReference2005_Private(const ActiveXReference2005_Private &ref);
    ActiveXReference2005_Private& operator=(const ActiveXReference2005_Private &ref);
    void processNodeAttributes(const QDomElement &element);
    ReferenceConfiguration::Ptr createReferenceConfiguration() const;

    QString m_localeID;         // opt
    QString m_copyLocal;        // opt
    bool m_useInBuild;          // opt, default "true"
};

class ActiveXReference2008_Private : public ActiveXReference2005_Private
{
    friend class ActiveXReference2008;

public:
    ~ActiveXReference2008_Private();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;
    ActiveXReference_Private::Ptr clone() const;

    bool copyLocalDependencies() const;
    void setCopyLocalDependencies(bool copyLocalDependencies);
    bool copyLocalSatelliteAssemblies() const;
    void setCopyLocalSatelliteAssemblies(bool copyLocalSatelliteAssemblies);
    bool useDependenciesInBuild() const;
    void setUseDependenciesInBuild(bool useDependenciesInBuild);

protected:
    ActiveXReference2008_Private();
    ActiveXReference2008_Private(const ActiveXReference2008_Private &ref);
    ActiveXReference2008_Private& operator=(const ActiveXReference2008_Private &ref);
    void processNodeAttributes(const QDomElement &element);
    ReferenceConfiguration::Ptr createReferenceConfiguration() const;

    bool m_copyLocalDependencies; //optional    default: true
    bool m_copyLocalSatelliteAssemblies; //optional     default: true
    bool m_useDependenciesInBuild; //optional   default: true
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_ACTIVEXREFERENCE_PRIVATE_H
