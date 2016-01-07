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

#include "voidelementtasks.h"

namespace qmt {

VoidElementTasks::VoidElementTasks()
{
}

VoidElementTasks::~VoidElementTasks()
{
}

void VoidElementTasks::openElement(const MElement *)
{
}

void VoidElementTasks::openElement(const DElement *, const MDiagram *)
{
}

bool VoidElementTasks::hasClassDefinition(const MElement *) const
{
    return false;
}

bool VoidElementTasks::hasClassDefinition(const DElement *, const MDiagram *) const
{
    return false;
}

void VoidElementTasks::openClassDefinition(const MElement *)
{
}

void VoidElementTasks::openClassDefinition(const DElement *, const MDiagram *)
{
}

bool VoidElementTasks::hasHeaderFile(const MElement *) const
{
    return false;
}

bool VoidElementTasks::hasHeaderFile(const DElement *, const MDiagram *) const
{
    return false;
}

bool VoidElementTasks::hasSourceFile(const MElement *) const
{
    return false;
}

bool VoidElementTasks::hasSourceFile(const DElement *, const MDiagram *) const
{
    return false;
}

void VoidElementTasks::openHeaderFile(const MElement *)
{
}

void VoidElementTasks::openHeaderFile(const DElement *, const MDiagram *)
{
}

void VoidElementTasks::openSourceFile(const MElement *)
{
}

void VoidElementTasks::openSourceFile(const DElement *, const MDiagram *)
{
}

bool VoidElementTasks::hasFolder(const MElement *) const
{
    return false;
}

bool VoidElementTasks::hasFolder(const DElement *, const MDiagram *) const
{
    return false;
}

void VoidElementTasks::showFolder(const MElement *)
{
}

void VoidElementTasks::showFolder(const DElement *, const MDiagram *)
{
}

bool VoidElementTasks::hasDiagram(const MElement *) const
{
    return false;
}

bool VoidElementTasks::hasDiagram(const DElement *, const MDiagram *) const
{
    return false;
}

void VoidElementTasks::openDiagram(const MElement *)
{
}

void VoidElementTasks::openDiagram(const DElement *, const MDiagram *)
{
}

bool VoidElementTasks::hasParentDiagram(const MElement *) const
{
    return false;
}

bool VoidElementTasks::hasParentDiagram(const DElement *, const MDiagram *) const
{
    return false;
}

void VoidElementTasks::openParentDiagram(const MElement *)
{
}

void VoidElementTasks::openParentDiagram(const DElement *, const MElement *)
{
}

bool VoidElementTasks::mayCreateDiagram(const MElement *) const
{
    return false;
}

bool VoidElementTasks::mayCreateDiagram(const DElement *, const MDiagram *) const
{
    return false;
}

void VoidElementTasks::createAndOpenDiagram(const MElement *)
{
}

void VoidElementTasks::createAndOpenDiagram(const DElement *, const MDiagram *)
{
}

} // namespace qmt
