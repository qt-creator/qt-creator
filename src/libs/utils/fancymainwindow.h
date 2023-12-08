// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "store.h"

#include <QMainWindow>

namespace Utils {

struct FancyMainWindowPrivate;
class QtcSettings;

class QTCREATOR_UTILS_EXPORT FancyMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FancyMainWindow(QWidget *parent = nullptr);
    ~FancyMainWindow() override;

    /* The widget passed in should have an objectname set
     * which will then be used as key for QSettings. */
    QDockWidget *addDockForWidget(QWidget *widget, bool immutable = false);
    const QList<QDockWidget *> dockWidgets() const;
    QList<QDockWidget *> docksInArea(Qt::DockWidgetArea area) const;

    void setTrackingEnabled(bool enabled);

    void saveSettings(QtcSettings *settings) const;
    void restoreSettings(const QtcSettings *settings);
    Store saveSettings() const;
    bool restoreSettings(const Store &settings);
    bool restoreFancyState(const QByteArray &state, int version = 0);

    // Additional context menu actions
    QAction *menuSeparator1() const;
    QAction *resetLayoutAction() const;
    QAction *showCentralWidgetAction() const;
    void addDockActionsToMenu(QMenu *menu);

    bool isCentralWidgetShown() const;
    void showCentralWidget(bool on);

    void setDockAreaVisible(Qt::DockWidgetArea area, bool visible);
    bool isDockAreaVisible(Qt::DockWidgetArea area) const;
    bool isDockAreaAvailable(Qt::DockWidgetArea area) const;

signals:
    // Emitted by resetLayoutAction(). Connect to a slot
    // restoring the default layout.
    void resetLayout();
    void dockWidgetsChanged();

public slots:
    void setDockActionsVisible(bool v);

protected:
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void handleVisibilityChanged(bool visible);

    FancyMainWindowPrivate *d;
};

} // namespace Utils
