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
