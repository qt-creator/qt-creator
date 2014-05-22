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
#include "activexreference.h"
#include "assemblyreference.h"
#include "generalattributecontainer.h"
#include "generalattributecontainer.h"
#include "projectreference.h"
#include "references.h"

#include <utils/qtcassert.h>

#include <QDomNode>

namespace VcProjectManager {
namespace Internal {

References::References()
{
}

References::References(const References &references)
{
    foreach (const IReference *ref, references.m_references)
        m_references.append(ref->clone());
}

References &References::operator =(const References &references)
{
    if (this != &references) {
        qDeleteAll(m_references);
        m_references.clear();

        foreach (const IReference *ref, references.m_references)
            m_references.append(ref->clone());
    }
    return *this;
}

References::~References()
{
    qDeleteAll(m_references);
}

void References::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.hasChildNodes()) {
        QDomNode firstChild = node.firstChild();
        if (!firstChild.isNull())
            processReference(firstChild);
    }
}

VcNodeWidget *References::createSettingsWidget()
{
    return 0;
}

QDomNode References::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement fileNode = domXMLDocument.createElement(QLatin1String("References"));

    foreach (const IReference *asmRef, m_references)
        fileNode.appendChild(asmRef->toXMLDomNode(domXMLDocument));

    return fileNode;
}

void References::addReference(IReference *reference)
{
    if (!reference || m_references.contains(reference))
        return;
    m_references.append(reference);
}

void References::removeReference(IReference *reference)
{
    m_references.removeOne(reference);
    delete reference;
}

int References::referenceCount() const
{
    return m_references.size();
}

IReference *References::reference(int index) const
{
    QTC_ASSERT(0 <= index && index < m_references.size(), return 0);
    return m_references[index];
}

void References::processReference(const QDomNode &referenceNode)
{
    IReference *reference = 0;
    if (referenceNode.nodeName() == QLatin1String("AssemblyReference"))
        reference = new AssemblyReference;

    else if (referenceNode.nodeName() == QLatin1String("ActiveXReference"))
        reference = new ActiveXReference;

    else if (referenceNode.nodeName() == QLatin1String("ProjectReference"))
        reference = new ProjectReference;

    if (reference) {
        reference->processNode(referenceNode);
        m_references.append(reference);
    }

    // process next sibling
    QDomNode nextSibling = referenceNode.nextSibling();
    if (!nextSibling.isNull())
        processReference(nextSibling);
}

} // namespace Internal
} // namespace VcProjectManager
