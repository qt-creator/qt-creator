/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef ANALYZERMANAGER_H
#define ANALYZERMANAGER_H

#include "analyzerbase_global.h"
#include "analyzerconstants.h"
#include <coreplugin/id.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QDockWidget;
class QAction;
QT_END_NAMESPACE

namespace Utils {
class FancyMainWindow;
}

namespace Analyzer {

typedef QList<StartMode> StartModes;

class IAnalyzerTool;
class AnalyzerManagerPrivate;


// FIXME: Merge with AnalyzerPlugin.
class ANALYZER_EXPORT AnalyzerManager : public QObject
{
    Q_OBJECT

public:
    explicit AnalyzerManager(QObject *parent);
    ~AnalyzerManager();

    static void extensionsInitialized();
    static void shutdown();

    // Register a tool and initialize it.
    static void addTool(IAnalyzerTool *tool, const StartModes &mode);
    static IAnalyzerTool *toolFromRunMode(ProjectExplorer::RunMode runMode);

    // Dockwidgets are registered to the main window.
    static QDockWidget *createDockWidget(IAnalyzerTool *tool, const QString &title,
        QWidget *widget, Qt::DockWidgetArea area = Qt::TopDockWidgetArea);

    static Utils::FancyMainWindow *mainWindow();

    static void showMode();
    static IAnalyzerTool *currentSelectedTool();
    static QList<IAnalyzerTool *> tools();
    static void selectTool(IAnalyzerTool *tool, StartMode mode);
    static void startTool(IAnalyzerTool *tool, StartMode mode);
    static void stopTool();

    // Convenience functions.
    static void startLocalTool(IAnalyzerTool *tool);

    static QString msgToolStarted(const QString &name);
    static QString msgToolFinished(const QString &name, int issuesFound);

    static void showStatusMessage(const QString &message, int timeoutMS = 10000);
    static void showPermanentStatusMessage(const QString &message);

    static void handleToolStarted();
    static void handleToolFinished();
    static QAction *stopAction();

private:
    friend class AnalyzerManagerPrivate;
    AnalyzerManagerPrivate *const d;
};

} // namespace Analyzer

#endif // ANALYZERMANAGER_H
