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

#ifndef QMT_TOOLBAR_H
#define QMT_TOOLBAR_H

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
            : m_toolType(TooltypeTool),
              m_name(name),
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
    int m_priority = -1;
    QList<Tool> m_tools;
};

} // namespace qmt

#endif // QMT_TOOLBAR_H
