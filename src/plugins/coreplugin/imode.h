// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "icontext.h"

#include <utils/id.h>

#include <QIcon>
#include <QMenu>

namespace Core {

class CORE_EXPORT IMode : public IContext
{
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName)
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon)
    Q_PROPERTY(int priority READ priority WRITE setPriority)
    Q_PROPERTY(Utils::Id id READ id WRITE setId)
    Q_PROPERTY(QMenu *menu READ menu WRITE setMenu)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledStateChanged)

public:
    IMode(QObject *parent = nullptr);

    QString displayName() const { return m_displayName; }
    QIcon icon() const { return m_icon; }
    int priority() const { return m_priority; }
    Utils::Id id() const { return m_id; }
    bool isEnabled() const;
    QMenu *menu() const { return m_menu; }

    void setEnabled(bool enabled);
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setIcon(const QIcon &icon) { m_icon = icon; }
    void setPriority(int priority) { m_priority = priority; }
    void setId(Utils::Id id) { m_id = id; }
    void setMenu(QMenu *menu) { m_menu = menu; }

signals:
    void enabledStateChanged(bool enabled);

private:
    QString m_displayName;
    QIcon m_icon;
    QMenu *m_menu = nullptr;
    int m_priority = -1;
    Utils::Id m_id;
    bool m_isEnabled = true;
};

} // namespace Core
