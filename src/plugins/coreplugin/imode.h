// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "icontext.h"

#include <utils/id.h>

#include <QIcon>
#include <QMenu>

#include <functional>
#include <memory.h>

namespace Utils {
class FancyMainWindow;
}

namespace Core {

namespace Internal {
class IModePrivate;
}

class CORE_EXPORT IMode : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName)
    Q_PROPERTY(QIcon icon READ icon WRITE setIcon)
    Q_PROPERTY(int priority READ priority WRITE setPriority)
    Q_PROPERTY(Utils::Id id READ id WRITE setId)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledStateChanged)

public:
    IMode(QObject *parent = nullptr);
    ~IMode();

    QString displayName() const;
    QIcon icon() const;
    int priority() const;
    Utils::Id id() const;
    bool isEnabled() const;
    bool isVisible() const;
    bool hasMenu() const;
    void addToMenu(QMenu *menu) const;
    Context context() const;
    QWidget *widget() const;

    void setEnabled(bool enabled);
    void setVisible(bool visible);
    void setDisplayName(const QString &displayName);
    void setIcon(const QIcon &icon);
    void setPriority(int priority);
    void setId(Utils::Id id);
    void setMenu(std::function<void(QMenu *)> menuFunction);
    void setContext(const Context &context);
    void setWidget(QWidget *widget);
    void setWidgetCreator(const std::function<QWidget *()> &widgetCreator);

    Utils::FancyMainWindow *mainWindow();
    void setMainWindow(Utils::FancyMainWindow *mw);

signals:
    void enabledStateChanged(bool enabled);
    void visibleChanged(bool visible);

private:
    std::unique_ptr<Internal::IModePrivate> m_d;
};

} // namespace Core
