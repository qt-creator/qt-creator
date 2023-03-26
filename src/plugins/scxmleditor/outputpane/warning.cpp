// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "warning.h"

using namespace ScxmlEditor::OutputPane;

Warning::Warning(Severity severity, const QString &name, const QString &reason,
    const QString &description, bool active, QObject *parent)
    : QObject(parent)
    , m_severity(severity)
    , m_typeName(name)
    , m_reason(reason)
    , m_description(description)
    , m_active(active)
{
}

void Warning::setSeverity(Warning::Severity sev)
{
    if (m_severity != sev) {
        m_severity = sev;
        emit dataChanged();
    }
}
void Warning::setTypeName(const QString &name)
{
    if (m_typeName != name) {
        m_typeName = name;
        emit dataChanged();
    }
}
void Warning::setReason(const QString &reason)
{
    if (m_reason != reason) {
        m_reason = reason;
        emit dataChanged();
    }
}
void Warning::setDescription(const QString &description)
{
    if (m_description != description) {
        m_description = description;
        emit dataChanged();
    }
}
void Warning::setActive(bool active)
{
    if (m_active != active) {
        m_active = active;
        emit dataChanged();
    }
}
