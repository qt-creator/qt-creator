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
#include "globals.h"

#include <utils/qtcassert.h>

namespace VcProjectManager {
namespace Internal {

Globals::Globals()
{
}

Globals::Globals(const Globals &globals)
{
    foreach (IGlobal *global, globals.m_globals)
        m_globals.append(global->clone());
}

Globals &Globals::operator =(const Globals &globals)
{
    if (this != &globals) {
        qDeleteAll(m_globals);
        m_globals.clear();

        foreach (IGlobal *global, globals.m_globals)
            m_globals.append(global->clone());
    }

    return *this;
}

Globals::~Globals()
{
    qDeleteAll(m_globals);
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

VcNodeWidget *Globals::createSettingsWidget()
{
    return 0;
}

QDomNode Globals::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement globalsNode = domXMLDocument.createElement(QLatin1String("Globals"));

    foreach (const IGlobal *global, m_globals)
        globalsNode.appendChild(global->toXMLDomNode(domXMLDocument));

    return globalsNode;
}

void Globals::addGlobal(IGlobal *global)
{
    if (m_globals.contains(global))
        m_globals.append(global);
}

int Globals::globalCount() const
{
    return m_globals.size();
}

IGlobal *Globals::global(int index) const
{
    QTC_ASSERT(0 <= index && index < m_globals.size(), return 0);
    return m_globals[index];
}

void Globals::removeGlobal(IGlobal *global)
{
    m_globals.removeOne(global);
}

void Globals::processGlobal(const QDomNode &globalNode)
{
    Global* global = new Global;
    global->processNode(globalNode);
    m_globals.append(global);

    // process next sibling
    QDomNode nextSibling = globalNode.nextSibling();
    if (!nextSibling.isNull())
        processGlobal(nextSibling);
}

} // namespace Internal
} // namespace VcProjectManager
