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

#pragma once

#include "qmt/infrastructure/qmt_global.h"

#include <QString>
#include <QList>

namespace qmt {

class QMT_EXPORT Toolbar
{
public:
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
             const QString &stereotype = QString::null)
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

    QString id() const { return m_id; }
    void setId(const QString &id);
    int priority() const { return m_priority; }
    void setPriority(int priority);
    QList<Tool> tools() const { return m_tools; }
    void setTools(const QList<Tool> &tools);

private:
    QString m_id;
    int m_priority;
    QList<Tool> m_tools;
};

} // namespace qmt
