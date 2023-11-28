// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "icontext.h"

#include <utils/id.h>

#include <QIcon>
#include <QMenu>

#include <memory.h>

namespace Utils {
class FancyMainWindow;
}

namespace Core {

namespace Internal {
class IModePrivate;
}

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
    ~IMode();

    QString displayName() const;
    QIcon icon() const;
    int priority() const;
    Utils::Id id() const;
    bool isEnabled() const;
    QMenu *menu() const;

    void setEnabled(bool enabled);
    void setDisplayName(const QString &displayName);
    void setIcon(const QIcon &icon);
    void setPriority(int priority);
    void setId(Utils::Id id);
    void setMenu(QMenu *menu);

    virtual Utils::FancyMainWindow *mainWindow();

signals:
    void enabledStateChanged(bool enabled);

private:
    std::unique_ptr<Internal::IModePrivate> m_d;
};

} // namespace Core
