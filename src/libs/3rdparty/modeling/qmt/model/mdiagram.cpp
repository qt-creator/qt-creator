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
      _last_modified()
{
}

MDiagram::MDiagram(const MDiagram &rhs)
    : MObject(rhs),
      _elements(),
      // modification date is copied (instead of set to current time) to allow exact copies of the diagram
      _last_modified(rhs._last_modified)
{
}

MDiagram::~MDiagram()
{
    qDeleteAll(_elements);
}

MDiagram &MDiagram::operator=(const MDiagram &rhs)
{
    if (this != &rhs) {
        MObject::operator=(rhs);
        // no deep copy; list of elements remains unchanged
        // modification date is copied (instead of set to current time) to allow exact copies of the diagram
        _last_modified = rhs._last_modified;
    }
    return *this;
}

DElement *MDiagram::findDiagramElement(const Uid &key) const
{
    // PERFORM introduce map for better performance
    foreach (DElement *element, _elements) {
        if (element->getUid() == key) {
            return element;
        }
    }
    return 0;
}

void MDiagram::setDiagramElements(const QList<DElement *> &elements)
{
    _elements = elements;
}

void MDiagram::addDiagramElement(DElement *element)
{
    QMT_CHECK(element);

    _elements.append(element);
}

void MDiagram::insertDiagramElement(int before_element, DElement *element)
{
    QMT_CHECK(before_element >= 0 && before_element <= _elements.size());

    _elements.insert(before_element, element);
}

void MDiagram::removeDiagramElement(int index)
{
    QMT_CHECK(index >= 0 && index < _elements.size());

    delete _elements.at(index);
    _elements.removeAt(index);
}

void MDiagram::removeDiagramElement(DElement *element)
{
    QMT_CHECK(element);

    removeDiagramElement(_elements.indexOf(element));
}

void MDiagram::setLastModified(const QDateTime &last_modified)
{
    _last_modified = last_modified;
}

void MDiagram::setLastModifiedToNow()
{
    _last_modified = QDateTime::currentDateTime();
}

void MDiagram::accept(MVisitor *visitor)
{
    visitor->visitMDiagram(this);
}

void MDiagram::accept(MConstVisitor *visitor) const
{
    visitor->visitMDiagram(this);
}

}
