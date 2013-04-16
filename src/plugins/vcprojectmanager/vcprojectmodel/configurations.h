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
#ifndef VCPROJECTMANAGER_INTERNAL_CONFIGURATIONS_H
#define VCPROJECTMANAGER_INTERNAL_CONFIGURATIONS_H

#include "ivcprojectnodemodel.h"

#include "configuration.h"
#include "vcprojectdocument_constants.h"

namespace VcProjectManager {
namespace Internal {

class Configurations : public IVcProjectXMLNode
{
public:
    typedef QSharedPointer<Configurations>  Ptr;

    Configurations(VcDocConstants::DocumentVersion version);
    Configurations(const Configurations &configs);
    Configurations& operator=(const Configurations &configs);
    ~Configurations();

    void processNode(const QDomNode &node);
    void processNodeAttributes(const QDomElement &element);
    VcNodeWidget* createSettingsWidget();
    QDomNode toXMLDomNode(QDomDocument &domXMLDocument) const;

    bool isEmpty() const;

    Configuration::Ptr addConfiguration(const QString &configName);
    bool appendConfiguration(Configuration::Ptr config);
    void removeConfiguration(Configuration::Ptr config);
    Configuration::Ptr configuration(const QString &configName);
    Configuration::Ptr cloneConfiguration(const QString &newConfigName, const QString &configToClone);
    Configuration::Ptr cloneConfiguration(const QString &newConfigName, Configuration::Ptr config);
    QList<Configuration::Ptr > configurations() const;

private:
    void processConfiguration(const QDomNode &configurationNode);

    QList<Configuration::Ptr > m_configurations;
    VcDocConstants::DocumentVersion m_docVersion;
};

} // namespace Internal
} // namespace VcProjectManager

#endif // VCPROJECTMANAGER_INTERNAL_CONFIGURATIONS_H
