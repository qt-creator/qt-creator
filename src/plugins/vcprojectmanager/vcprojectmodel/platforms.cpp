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
#include "platforms.h"

#include <utils/qtcassert.h>

namespace VcProjectManager {
namespace Internal {

Platforms::Platforms()
{
}

Platforms::Platforms(const Platforms &platforms)
{
    foreach (const IPlatform *platform, platforms.m_platforms)
        m_platforms.append(platform->clone());
}

Platforms &Platforms::operator =(const Platforms &platforms)
{
    if (this != &platforms) {
        qDeleteAll(m_platforms);
        m_platforms.clear();

        foreach (const IPlatform *platform, platforms.m_platforms)
            m_platforms.append(platform->clone());
    }
    return *this;
}

Platforms::~Platforms()
{
    qDeleteAll(m_platforms);
}

void Platforms::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.hasChildNodes()) {
        QDomNode firstChild = node.firstChild();
        if (!firstChild.isNull())
            processPlatform(firstChild);
    }
}

VcNodeWidget *Platforms::createSettingsWidget()
{
    return 0;
}

QDomNode Platforms::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement platformsNode = domXMLDocument.createElement(QLatin1String("Platforms"));

    foreach (const IPlatform *platform, m_platforms)
        platformsNode.appendChild(platform->toXMLDomNode(domXMLDocument));

    return platformsNode;
}

void Platforms::addPlatform(IPlatform *platform)
{
    if (!platform || m_platforms.contains(platform))
        return;
    foreach (const IPlatform *plat, m_platforms) {
        if (plat && platform && plat->displayName() == platform->displayName())
            return;
    }

    m_platforms.append(platform);
}

int Platforms::platformCount() const
{
    return m_platforms.size();
}

IPlatform *Platforms::platform(int index) const
{
    QTC_ASSERT(0 <= index && index < m_platforms.size(), return 0);
    return m_platforms[index];
}

void Platforms::removePlatform(IPlatform *platform)
{
    if (!platform || !m_platforms.contains(platform))
        return;
    foreach (const IPlatform *plat, m_platforms) {
        if (plat && platform && plat->displayName() == platform->displayName()) {
            m_platforms.removeOne(platform);
            delete platform;
        }
    }
}

void Platforms::processPlatform(const QDomNode &node)
{
    IPlatform *platform = new Platform;
    platform->processNode(node);
    m_platforms.append(platform);

    // process next sibling
    QDomNode nextSibling = node.nextSibling();
    if (!nextSibling.isNull())
        processPlatform(nextSibling);
}

} // namespace Internal
} // namespace VcProjectManager
