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
#include "references.h"

namespace VcProjectManager {
namespace Internal {

References::References(VcDocConstants::DocumentVersion version)
    : m_docVersion(version)
{
}

References::References(const References &references)
{
    m_docVersion = references.m_docVersion;

    foreach (const ActiveXReference::Ptr &ref, references.m_activeXReferences)
        m_activeXReferences.append(ref->clone());

    foreach (const AssemblyReference::Ptr &ref, references.m_assemblyReferences)
        m_assemblyReferences.append(ref->clone());

    foreach (const ProjectReference::Ptr &ref, references.m_projectReferences)
        m_projectReferences.append(ref->clone());
}

References &References::operator =(const References &references)
{
    if (this != &references) {
        m_docVersion = references.m_docVersion;
        m_activeXReferences.clear();
        m_assemblyReferences.clear();
        m_projectReferences.clear();

        foreach (const ActiveXReference::Ptr &ref, references.m_activeXReferences)
            m_activeXReferences.append(ref->clone());

        foreach (const AssemblyReference::Ptr &ref, references.m_assemblyReferences)
            m_assemblyReferences.append(ref->clone());

        foreach (const ProjectReference::Ptr &ref, references.m_projectReferences)
            m_projectReferences.append(ref->clone());
    }
    return *this;
}

References::~References()
{
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

    foreach (const AssemblyReference::Ptr &asmRef, m_assemblyReferences)
        fileNode.appendChild(asmRef->toXMLDomNode(domXMLDocument));

    foreach (const ActiveXReference::Ptr &activeXRef, m_activeXReferences)
        fileNode.appendChild(activeXRef->toXMLDomNode(domXMLDocument));

    foreach (const ProjectReference::Ptr &projRef, m_projectReferences)
        fileNode.appendChild(projRef->toXMLDomNode(domXMLDocument));

    return fileNode;
}

bool References::isEmpty() const
{
    return m_activeXReferences.isEmpty() && m_assemblyReferences.isEmpty() && m_projectReferences.isEmpty();
}

void References::addAssemblyReference(AssemblyReference::Ptr asmRef)
{
    if (m_assemblyReferences.contains(asmRef))
        return;
    m_assemblyReferences.append(asmRef);
}

void References::removeAssemblyReference(AssemblyReference::Ptr asmRef)
{
    m_assemblyReferences.removeAll(asmRef);
}

void References::addActiveXReference(ActiveXReference::Ptr actXRef)
{
    if (m_activeXReferences.contains(actXRef))
        return;
    m_activeXReferences.append(actXRef);
}

void References::removeActiveXReference(ActiveXReference::Ptr actXRef)
{
    m_activeXReferences.removeAll(actXRef);
}

void References::addProjectReference(ProjectReference::Ptr projRefer)
{
    if (m_projectReferences.contains(projRefer))
        return;
    m_projectReferences.append(projRefer);
}

void References::removeProjectReference(ProjectReference::Ptr projRef)
{
    m_projectReferences.removeAll(projRef);
}

void References::removeProjectReference(const QString &projRefName)
{
    foreach (const ProjectReference::Ptr &projRef, m_projectReferences) {
        if (projRef->name() == projRefName) {
            removeProjectReference(projRef);
            return;
        }
    }
}

void References::processReference(const QDomNode &referenceNode)
{
    if (referenceNode.nodeName() == QLatin1String("AssemblyReference")) {
        AssemblyReference::Ptr reference = AssemblyReferenceFactory::instance().create(m_docVersion);
        m_assemblyReferences.append(reference);
        reference->processNode(referenceNode);
    }

    else if (referenceNode.nodeName() == QLatin1String("ActiveXReference")) {
        ActiveXReference::Ptr reference = ActiveXReferenceFactory::instance().create(m_docVersion);
        m_activeXReferences.append(reference);
        reference->processNode(referenceNode);
    }

    else if (referenceNode.nodeName() == QLatin1String("ProjectReference")) {
        ProjectReference::Ptr reference = ProjectReferenceFactory::instance().create(m_docVersion);
        m_projectReferences.append(reference);
        reference->processNode(referenceNode);
    }

    // process next sibling
    QDomNode nextSibling = referenceNode.nextSibling();
    if (!nextSibling.isNull())
        processReference(nextSibling);
}

} // namespace Internal
} // namespace VcProjectManager
