// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
