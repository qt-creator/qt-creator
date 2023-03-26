// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/qmt_global.h"

#include <QString>
#include <QList>

namespace qmt {

class QMT_EXPORT Toolbar
{
public:
    enum ToolbarType {
        ObjectToolbar,
        RelationToolbar
    };

    enum ToolType {
        TooltypeTool,
        TooltypeSeparator
    };

    class Tool
    {
    public:
        Tool()
            : m_toolType(TooltypeSeparator)
        {
        }

        Tool(const QString &name, const QString &elementType,
             const QString &stereotype = QString())
            : m_name(name),
              m_elementType(elementType),
              m_stereotype(stereotype)
        {
        }

        ToolType m_toolType = ToolType::TooltypeTool;
        QString m_name;
        QString m_elementType;
        QString m_stereotype;
    };

    Toolbar();
    ~Toolbar();

    ToolbarType toolbarType() const { return m_toolbarType; }
    void setToolbarType(ToolbarType toolbarType);
    QString id() const { return m_id; }
    void setId(const QString &id);
    int priority() const { return m_priority; }
    void setPriority(int priority);
    QStringList elementTypes() const { return m_elementTypes; }
    void setElementTypes(const QStringList &elementTypes);
    QList<Tool> tools() const { return m_tools; }
    void setTools(const QList<Tool> &tools);

private:
    ToolbarType m_toolbarType = ObjectToolbar;
    QString m_id;
    int m_priority = -1;
    QStringList m_elementTypes;
    QList<Tool> m_tools;
};

} // namespace qmt
