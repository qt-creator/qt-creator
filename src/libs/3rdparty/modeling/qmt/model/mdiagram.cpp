/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
      m_elements(),
      // modification date is copied (instead of set to current time) to allow exact copies of the diagram
      m_lastModified(rhs.m_lastModified)
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
    }
    return *this;
}

DElement *MDiagram::findDiagramElement(const Uid &key) const
{
    // PERFORM introduce map for better performance
    foreach (DElement *element, m_elements) {
        if (element->uid() == key)
            return element;
    }
    return 0;
}

void MDiagram::setDiagramElements(const QList<DElement *> &elements)
{
    m_elements = elements;
}

void MDiagram::addDiagramElement(DElement *element)
{
    QMT_CHECK(element);

    m_elements.append(element);
}

void MDiagram::insertDiagramElement(int beforeElement, DElement *element)
{
    QMT_CHECK(beforeElement >= 0 && beforeElement <= m_elements.size());

    m_elements.insert(beforeElement, element);
}

void MDiagram::removeDiagramElement(int index)
{
    QMT_CHECK(index >= 0 && index < m_elements.size());

    delete m_elements.at(index);
    m_elements.removeAt(index);
}

void MDiagram::removeDiagramElement(DElement *element)
{
    QMT_CHECK(element);

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

void MDiagram::accept(MVisitor *visitor)
{
    visitor->visitMDiagram(this);
}

void MDiagram::accept(MConstVisitor *visitor) const
{
    visitor->visitMDiagram(this);
}

} // namespace qmt
