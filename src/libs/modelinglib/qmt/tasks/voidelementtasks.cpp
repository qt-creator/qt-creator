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

bool VoidElementTasks::extendContextMenu(const DElement *, const MDiagram *, QMenu *)
{
    return false;
}

bool VoidElementTasks::handleContextMenuAction(const DElement *, const MDiagram *, const QString &)
{
    return false;
}

} // namespace qmt
