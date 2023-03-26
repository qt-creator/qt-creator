// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolbar.h"

namespace qmt {

Toolbar::Toolbar()
{
}

Toolbar::~Toolbar()
{
}

void Toolbar::setToolbarType(Toolbar::ToolbarType toolbarType)
{
    m_toolbarType = toolbarType;
}

void Toolbar::setId(const QString &id)
{
    m_id = id;
}

void Toolbar::setPriority(int priority)
{
    m_priority = priority;
}

void Toolbar::setElementTypes(const QStringList &elementTypes)
{
    m_elementTypes = elementTypes;
}

void Toolbar::setTools(const QList<Toolbar::Tool> &tools)
{
    m_tools = tools;
}

} // namespace qmt
