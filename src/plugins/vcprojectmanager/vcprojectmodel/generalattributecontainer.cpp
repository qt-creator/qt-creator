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
#include "generalattributecontainer.h"

namespace VcProjectManager {
namespace Internal {

GeneralAttributeContainer::GeneralAttributeContainer()
{
}

GeneralAttributeContainer::GeneralAttributeContainer(const GeneralAttributeContainer &attrCont)
{
    m_anyAttribute = attrCont.m_anyAttribute;
}

GeneralAttributeContainer &GeneralAttributeContainer::operator=(const GeneralAttributeContainer &attrCont)
{
    if (this != &attrCont) {
        m_anyAttribute.clear();
        for (int i = 0; i < attrCont.getAttributeCount(); ++i) {
            QString attrName = attrCont.getAttributeName(i);
            m_anyAttribute.insert(attrName, attrCont.attributeValue(attrName));
        }
    }

    return *this;
}

QString GeneralAttributeContainer::attributeValue(const QString &attributeName) const
{
    return m_anyAttribute.value(attributeName);
}

void GeneralAttributeContainer::clearAttribute(const QString &attributeName)
{
    if (m_anyAttribute.contains(attributeName))
        m_anyAttribute.insert(attributeName, QString());
}

void GeneralAttributeContainer::removeAttribute(const QString &attributeName)
{
    m_anyAttribute.remove(attributeName);
}

void GeneralAttributeContainer::setAttribute(const QString &attributeName, const QString &attributeValue)
{
    m_anyAttribute.insert(attributeName, attributeValue);
}

QString GeneralAttributeContainer::getAttributeName(int index) const
{
    QHashIterator<QString, QString> it(m_anyAttribute);

    index++;

    while (it.hasNext() && index) {
        it.next();
        --index;
    }

    if (!index)
        return it.key();

    return QString();
}

int GeneralAttributeContainer::getAttributeCount() const
{
    return m_anyAttribute.size();
}

void GeneralAttributeContainer::appendToXMLNode(QDomElement &elementNode) const
{
    QHashIterator<QString, QString> it(m_anyAttribute);

    while (it.hasNext()) {
        it.next();
        elementNode.setAttribute(it.key(), it.value());
    }
}

} // namespace Internal
} // namespace VcProjectManager
