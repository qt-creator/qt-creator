/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "mdiagram.h"

#include "mvisitor.h"
#include "mconstvisitor.h"
#include "qmt/diagram/delement.h"

namespace qmt {

MDiagram::MDiagram()
    : MObject(),
      // modification date is set to null value for performance reasons
      m_lastModified()
{
}

MDiagram::MDiagram(const MDiagram &rhs)
    : MObject(rhs),
      // no deep copy
      // modification date is copied (instead of set to current time) to allow exact copies of the diagram
      m_lastModified(rhs.m_lastModified),
      m_toolbarId(rhs.toolbarId())
{
}

MDiagram::~MDiagram()
{
    qDeleteAll(m_elements);
}

MDiagram &MDiagram::operator=(const MDiagram &rhs)
{
    if (this != &rhs) {
        MObject::operator=(rhs);
        // no deep copy; list of elements remains unchanged
        // modification date is copied (instead of set to current time) to allow exact copies of the diagram
        m_lastModified = rhs.m_lastModified;
        m_toolbarId = rhs.m_toolbarId;
    }
    return *this;
}

DElement *MDiagram::findDiagramElement(const Uid &key) const
{
    return m_elementMap.value(key);
}

DElement *MDiagram::findDelegate(const Uid &modelUid) const
{
    return m_modelUid2ElementMap.value(modelUid);
}

void MDiagram::setDiagramElements(const QList<DElement *> &elements)
{
    m_elements = elements;
    m_elementMap.clear();
    m_modelUid2ElementMap.clear();
    for (DElement *element : elements) {
        m_elementMap.insert(element->uid(), element);
        m_modelUid2ElementMap.insert(element->modelUid(), element);
    }
}

void MDiagram::addDiagramElement(DElement *element)
{
    QMT_ASSERT(element, return);

    m_elements.append(element);
    m_elementMap.insert(element->uid(), element);
    m_modelUid2ElementMap.insert(element->modelUid(), element);
}

void MDiagram::insertDiagramElement(int beforeElement, DElement *element)
{
    QMT_ASSERT(beforeElement >= 0 && beforeElement <= m_elements.size(), return);

    m_elements.insert(beforeElement, element);
    m_elementMap.insert(element->uid(), element);
    m_modelUid2ElementMap.insert(element->modelUid(), element);
}

void MDiagram::removeDiagramElement(int index)
{
    QMT_ASSERT(index >= 0 && index < m_elements.size(), return);

    DElement *element = m_elements.at(index);
    m_elementMap.remove(element->uid());
    m_modelUid2ElementMap.remove(element->modelUid());
    delete element;
    m_elements.removeAt(index);
}

void MDiagram::removeDiagramElement(DElement *element)
{
    QMT_ASSERT(element, return);

    removeDiagramElement(m_elements.indexOf(element));
}

void MDiagram::setLastModified(const QDateTime &lastModified)
{
    m_lastModified = lastModified;
}

void MDiagram::setLastModifiedToNow()
{
    m_lastModified = QDateTime::currentDateTime();
}

void MDiagram::setToolbarId(const QString &toolbarId)
{
    m_toolbarId = toolbarId;
}

void MDiagram::accept(MVisitor *visitor)
{
    visitor->visitMDiagram(this);
}

void MDiagram::accept(MConstVisitor *visitor) const
{
    visitor->visitMDiagram(this);
}

} // namespace qmt
