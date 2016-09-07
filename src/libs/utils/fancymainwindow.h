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

#include "utils_global.h"

#include <QMainWindow>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils {

struct FancyMainWindowPrivate;

class QTCREATOR_UTILS_EXPORT FancyMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FancyMainWindow(QWidget *parent = 0);
    virtual ~FancyMainWindow();

    /* The widget passed in should have an objectname set
     * which will then be used as key for QSettings. */
    QDockWidget *addDockForWidget(QWidget *widget, bool immutable = false);
    QList<QDockWidget *> dockWidgets() const;

    void setTrackingEnabled(bool enabled);

    void saveSettings(QSettings *settings) const;
    void restoreSettings(const QSettings *settings);
    QHash<QString, QVariant> saveSettings() const;
    void restoreSettings(const QHash<QString, QVariant> &settings);

    // Additional context menu actions
    QAction *menuSeparator1() const;
    QAction *autoHideTitleBarsAction() const;
    QAction *menuSeparator2() const;
    QAction *resetLayoutAction() const;
    QAction *showCentralWidgetAction() const;
    void addDockActionsToMenu(QMenu *menu);

    bool autoHideTitleBars() const;

signals:
    // Emitted by resetLayoutAction(). Connect to a slot
    // restoring the default layout.
    void resetLayout();

public slots:
    void setDockActionsVisible(bool v);

protected:
    void hideEvent(QHideEvent *event);
    void showEvent(QShowEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

private:
    void onDockActionTriggered();

    void handleVisibilityChanged(bool visible);

    FancyMainWindowPrivate *d;
};

} // namespace Utils
