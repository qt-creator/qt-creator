// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "storekey.h"

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

    void setTrackingEnabled(bool enabled);

    void saveSettings(QtcSettings *settings) const;
    void restoreSettings(const QtcSettings *settings);
    QHash<Key, QVariant> saveSettings() const;
    void restoreSettings(const QHash<Key, QVariant> &settings);

    // Additional context menu actions
    QAction *menuSeparator1() const;
    QAction *autoHideTitleBarsAction() const;
    QAction *menuSeparator2() const;
    QAction *resetLayoutAction() const;
    QAction *showCentralWidgetAction() const;
    void addDockActionsToMenu(QMenu *menu);

    bool autoHideTitleBars() const;
    void setAutoHideTitleBars(bool on);

    bool isCentralWidgetShown() const;
    void showCentralWidget(bool on);

signals:
    // Emitted by resetLayoutAction(). Connect to a slot
    // restoring the default layout.
    void resetLayout();

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
