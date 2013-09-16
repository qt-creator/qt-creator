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
#include "platforms.h"

namespace VcProjectManager {
namespace Internal {

Platforms::Platforms()
{
}

Platforms::Platforms(const Platforms &platforms)
{
    foreach (const Platform::Ptr &platform, platforms.m_platforms)
        m_platforms.append(Platform::Ptr(new Platform(*platform)));
}

Platforms &Platforms::operator =(const Platforms &platforms)
{
    if (this != &platforms) {
        m_platforms.clear();

        foreach (const Platform::Ptr &platform, platforms.m_platforms)
            m_platforms.append(Platform::Ptr(new Platform(*platform)));
    }
    return *this;
}

Platforms::~Platforms()
{
    m_platforms.clear();
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

    foreach (const Platform::Ptr &platform, m_platforms)
        platformsNode.appendChild(platform->toXMLDomNode(domXMLDocument));

    return platformsNode;
}

bool Platforms::isEmpty() const
{
    return m_platforms.isEmpty();
}

void Platforms::processPlatform(const QDomNode &node)
{
    Platform::Ptr platform(new Platform);
    platform->processNode(node);
    m_platforms.append(platform);

    // process next sibling
    QDomNode nextSibling = node.nextSibling();
    if (!nextSibling.isNull())
        processPlatform(nextSibling);
}

void Platforms::addPlatform(Platform::Ptr platform)
{
    if (m_platforms.contains(platform))
        return;

    foreach (const Platform::Ptr &platf, m_platforms) {
        if (platf->name() == platform->name())
            return;
    }
    m_platforms.append(platform);
}

void Platforms::removePlatform(Platform::Ptr platform)
{
    m_platforms.removeAll(platform);
}

void Platforms::removePlatform(const QString &platformName)
{
    foreach (const Platform::Ptr &platform, m_platforms) {
        if (platform->name() == platformName) {
            removePlatform(platform);
            return;
        }
    }
}

QList<Platform::Ptr> Platforms::platforms() const
{
    return m_platforms;
}

} // namespace Internal
} // namespace VcProjectManager
