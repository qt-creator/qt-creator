// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
