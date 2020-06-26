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

#include "icontext.h"
#include "id.h"

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
    ~IMode() override;

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
