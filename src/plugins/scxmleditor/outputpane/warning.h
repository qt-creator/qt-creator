/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <QObject>
#include <QString>

namespace ScxmlEditor {

namespace OutputPane {

class Warning : public QObject
{
    Q_OBJECT

public:
    enum Severity {
        ErrorType = 0,
        WarningType,
        InfoType
    };

    explicit Warning(Severity severity, const QString &typeName,
        const QString &reason, const QString &description,
        bool active, QObject *parent = nullptr);

    Severity severity() const
    {
        return m_severity;
    }

    QString typeName() const
    {
        return m_typeName;
    }

    QString reason() const
    {
        return m_reason;
    }

    QString description() const
    {
        return m_description;
    }

    bool isActive() const
    {
        return m_active;
    }

    void setSeverity(Severity sev);
    void setTypeName(const QString &name);
    void setReason(const QString &reason);
    void setDescription(const QString &description);
    void setActive(bool active);

signals:
    void dataChanged();

private:
    Severity m_severity;
    QString m_typeName;
    QString m_reason;
    QString m_description;
    bool m_active;
};

} // namespace OutputPane
} // namespace ScxmlEditor
