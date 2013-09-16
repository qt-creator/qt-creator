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
#ifndef VCPROJECTMANAGER_INTERNAL_ACTIVEXREFERENCE_H
#define VCPROJECTMANAGER_INTERNAL_ACTIVEXREFERENCE_H

#include "ivcprojectnodemodel.h"
#include "vcprojectdocument_constants.h"
#include "configuration.h"

namespace VcProjectManager {
namespace Internal {

class ActiveXReference : public IVcProjectXMLNode
{
    friend class ActiveXReferenceFactory;

public:
    typedef QSharedPointer<ActiveXReference>    Ptr;

    virtual ~ActiveXReference();
    void processNode(const QDomNode &node);
    VcNodeWidget* createSettingsWidget();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    /*!
     * Implement in order to support creating a clone of a ActiveXReference instance.
     * \return A shared pointer to a newly created clone.
     */
    virtual ActiveXReference::Ptr clone() const = 0;

    void addReferenceConfiguration(Configuration::Ptr refConfig);
    void removeReferenceConfiguration(Configuration::Ptr refConfig);
    void removeReferenceConfiguration(const QString &refConfigName);
    QList<Configuration::Ptr> referenceConfigurations() const;
    Configuration::Ptr referenceConfiguration(const QString &refConfigName) const;

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
    virtual void processNodeAttributes(const QDomElement &element);
    virtual void processReferenceConfig(const QDomNode &referenceConfig);

    /*!
     * Reimplement this to create a new reference configuration.
     * \return A shared pointer to a newly created reference configuration.
     */
    virtual Configuration::Ptr createReferenceConfiguration() const = 0;

    QString m_controlGUID;  // required
    QString m_controlVersion;   // required
    QString m_wrapperTool;  // required
    QList<Configuration::Ptr> m_referenceConfigurations;
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
    Configuration::Ptr createReferenceConfiguration() const;
};

class ActiveXReference2005 : public ActiveXReference2003
{
    friend class ActiveXReferenceFactory;

public:
    ActiveXReference2005(const ActiveXReference2005 &ref);
    ActiveXReference2005& operator=(const ActiveXReference2005 &ref);
    ~ActiveXReference2005();

    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;
    ActiveXReference::Ptr clone() const;

    QString localeID() const;
    void setLocaleID(const QString &localeID);
    QString copyLocal() const;
    void setCopyLocal(const QString &copyLocal);
    bool useInBuild() const;
    void setUseInBuild(bool useInBuild);

protected:
    ActiveXReference2005();
    Configuration::Ptr createReferenceConfiguration() const;
    void processNodeAttributes(const QDomElement &element);

    QString m_localeID;         // opt
    QString m_copyLocal;        // opt
    bool m_useInBuild;          // opt, default "true"
};

class ActiveXReference2008 : public ActiveXReference2005
{
    friend class ActiveXReferenceFactory;

public:
    ActiveXReference2008(const ActiveXReference2008 &ref);
    ActiveXReference2008& operator=(const ActiveXReference2008 &ref);
    ~ActiveXReference2008();

    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;
    ActiveXReference::Ptr clone() const;

    bool copyLocalDependencies() const;
    void setCopyLocalDependencies(bool copyLocalDependencies);
    bool copyLocalSatelliteAssemblies() const;
    void setCopyLocalSatelliteAssemblies(bool copyLocalSatelliteAssemblies);
    bool useDependenciesInBuild() const;
    void setUseDependenciesInBuild(bool useDependenciesInBuild);

protected:
    ActiveXReference2008();
    Configuration::Ptr createReferenceConfiguration() const;
    void processNodeAttributes(const QDomElement &element);

    bool m_copyLocalDependencies; //optional    default: true
    bool m_copyLocalSatelliteAssemblies; //optional     default: true
    bool m_useDependenciesInBuild; //optional   default: true
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
