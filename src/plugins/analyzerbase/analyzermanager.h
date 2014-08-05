/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANALYZERMANAGER_H
#define ANALYZERMANAGER_H

#include "analyzerbase_global.h"
#include "analyzerconstants.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QDockWidget;
class QAction;
QT_END_NAMESPACE

namespace Utils { class FancyMainWindow; }
namespace ProjectExplorer { class RunConfiguration; }

namespace Analyzer {

class IAnalyzerTool;
class AnalyzerAction;
class AnalyzerRunControl;
class AnalyzerStartParameters;


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
    static QDockWidget *createDockWidget(IAnalyzerTool *tool,
        QWidget *widget, Qt::DockWidgetArea area = Qt::BottomDockWidgetArea);

    static Utils::FancyMainWindow *mainWindow();

    static void showMode();
    static void selectTool(IAnalyzerTool *tool, StartMode mode);
    static void startTool();
    static void stopTool();

    // Convenience functions.
    static void showStatusMessage(const QString &message, int timeoutMS = 10000);
    static void showPermanentStatusMessage(const QString &message);

    static void handleToolStarted();
    static void handleToolFinished();
    static QAction *stopAction();

    static AnalyzerRunControl *createRunControl(const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration);
};

} // namespace Analyzer

#endif // ANALYZERMANAGER_H
