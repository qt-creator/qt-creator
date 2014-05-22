/**************************************************************************
**
** Copyright (c) 2014 Bojan Petrovic
** Copyright (c) 2014 Radovan Zivkovic
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
#ifndef VCPROJECTMANAGER_INTERNAL_CONFIGURATION_H
#define VCPROJECTMANAGER_INTERNAL_CONFIGURATION_H

#include "../interfaces/iconfiguration.h"

QT_BEGIN_NAMESPACE
class QDomElement;
QT_END_NAMESPACE

namespace VcProjectManager {
namespace Internal {

class GeneralAttributeContainer;
class Tools;

class Configuration : public IConfiguration
{
public:
    explicit Configuration(const QString &nodeName);
    Configuration(const Configuration &config);
    Configuration& operator=(const Configuration &config);
    ~Configuration();

    void processNode(const QDomNode &node);
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    // IConfiguration interface
    IAttributeContainer *attributeContainer() const;
    QString fullName() const;
    QString displayname() const;
    QString platform() const;
    void setFullName(const QString &fullName);
    void setDisplayName(const QString &displayname);
    void setPlatform(const QString &platform);
    ITools *tools() const;
    IConfiguration *clone() const;
    VcNodeWidget *createSettingsWidget();

protected:
    virtual void processToolNode(const QDomNode &toolNode);
    void processNodeAttributes(const QDomElement &element);

    QString m_fullName;
    QString m_platformName;
    QString m_configurationName;
    QString m_nodeName;
    GeneralAttributeContainer *m_attributeContainer;
    Tools *m_tools;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_CONFIGURATION_H
