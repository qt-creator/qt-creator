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
#include "globals.h"

namespace VcProjectManager {
namespace Internal {

Globals::Globals()
{
}

Globals::Globals(const Globals &globals)
{
    foreach (Global::Ptr global, globals.m_globals)
        m_globals.append(Global::Ptr(new Global(*global)));
}

Globals &Globals::operator =(const Globals &globals)
{
    if (this != &globals) {
        m_globals.clear();

        foreach (Global::Ptr global, globals.m_globals)
            m_globals.append(Global::Ptr(new Global(*global)));
    }

    return *this;
}

Globals::~Globals()
{
    m_globals.clear();
}

void Globals::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.hasChildNodes()) {
        QDomNode firstChild = node.firstChild();

        if (!firstChild.isNull())
            processGlobal(firstChild);
    }
}

void Globals::processNodeAttributes(const QDomElement &element)
{
    Q_UNUSED(element)
}

VcNodeWidget *Globals::createSettingsWidget()
{
    return 0;
}

QDomNode Globals::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement globalsNode = domXMLDocument.createElement(QLatin1String("Globals"));

    foreach (const Global::Ptr &global, m_globals)
        globalsNode.appendChild(global->toXMLDomNode(domXMLDocument));

    return globalsNode;
}

bool Globals::isEmpty() const
{
    return m_globals.isEmpty();
}

void Globals::processGlobal(const QDomNode &globalNode)
{
    Global::Ptr global(new Global);
    global->processNode(globalNode);
    m_globals.append(global);

    // process next sibling
    QDomNode nextSibling = globalNode.nextSibling();
    if (!nextSibling.isNull())
        processGlobal(nextSibling);
}

void Globals::addGlobal(Global::Ptr global)
{
    if (m_globals.contains(global))
        m_globals.append(global);
}

void Globals::removeGlobal(Global::Ptr global)
{
    m_globals.removeAll(global);
}

void Globals::removeGlobal(const QString &globalName)
{
    foreach (const Global::Ptr &global, m_globals) {
        if (global->name() == globalName) {
            removeGlobal(global);
            return;
        }
    }
}

QList<Global::Ptr> Globals::globals() const
{
    return m_globals;
}

Global::Ptr Globals::global(const QString &name)
{
    foreach (const Global::Ptr &global, m_globals) {
        if (global->name() == name)
            return global;
    }
    return Global::Ptr();
}

} // namespace Internal
} // namespace VcProjectManager
