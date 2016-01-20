/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#ifndef ANALYZERMANAGER_H
#define ANALYZERMANAGER_H

#include "analyzerbase_global.h"

#include <projectexplorer/runconfiguration.h>

QT_BEGIN_NAMESPACE
class QDockWidget;
class QAction;
QT_END_NAMESPACE

namespace Utils { class FancyMainWindow; }

namespace Analyzer {

class AnalyzerAction;
class AnalyzerRunControl;


// FIXME: Merge with AnalyzerPlugin.
class ANALYZER_EXPORT AnalyzerManager : public QObject
{
    Q_OBJECT

public:
    explicit AnalyzerManager(QObject *parent);
    ~AnalyzerManager();

    static void shutdown();

    // Register a tool for a given start mode.
    static void addAction(AnalyzerAction *action);

    // Dockwidgets are registered to the main window.
    static QDockWidget *createDockWidget(Core::Id toolId,
        QWidget *widget, Qt::DockWidgetArea area = Qt::BottomDockWidgetArea);

    static Utils::FancyMainWindow *mainWindow();

    static void showMode();
    static void selectAction(Core::Id actionId, bool alsoRunIt = false);
    static void stopTool();

    // Convenience functions.
    static void showStatusMessage(Core::Id toolId, const QString &message, int timeoutMS = 10000);
    static void showPermanentStatusMessage(Core::Id toolId, const QString &message);

    static void handleToolStarted();
    static void handleToolFinished();
    static QAction *stopAction();

    static AnalyzerRunControl *createRunControl(
        ProjectExplorer::RunConfiguration *runConfiguration, Core::Id runMode);
};

} // namespace Analyzer

#endif // ANALYZERMANAGER_H
