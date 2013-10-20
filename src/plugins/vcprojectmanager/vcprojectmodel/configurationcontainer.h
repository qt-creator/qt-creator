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
#ifndef VCPROJECTMANAGER_INTERNAL_CONFIGURATIONCONTAINER_H
#define VCPROJECTMANAGER_INTERNAL_CONFIGURATIONCONTAINER_H

#include <QObject>
#include <QList>
#include <QDomElement>

namespace VcProjectManager {
namespace Internal {

class IConfiguration;

class ConfigurationContainer : public QObject
{
    Q_OBJECT

public:
    ConfigurationContainer(QObject *parent = 0);
    ConfigurationContainer(const ConfigurationContainer &configCont);
    ConfigurationContainer& operator=(const ConfigurationContainer &configCont);
    ~ConfigurationContainer();

    void addConfiguration(IConfiguration *config);
    IConfiguration* configuration(const QString &fullName) const;
    IConfiguration* configuration(int index) const;
    int configurationCount() const;
    void removeConfiguration(const QString &fullName);
    void appendToXMLNode(QDomElement &domElement, QDomDocument &domXMLDocument);

signals:
    void configurationAdded(IConfiguration *config);
    void configurationRemoved(QString fullConfigName);

private:
    QList<IConfiguration *> m_configs;
};

} // Internal
} // VcProjectManager
#endif // VCPROJECTMANAGER_INTERNAL_CONFIGURATIONCONTAINER_H
