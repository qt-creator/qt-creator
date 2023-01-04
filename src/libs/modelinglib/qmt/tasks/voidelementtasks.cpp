// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
