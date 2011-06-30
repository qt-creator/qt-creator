/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef ANALYZERMANAGER_H
#define ANALYZERMANAGER_H

#include "analyzerbase_global.h"
#include "projectexplorer/runconfiguration.h"

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE
class QDockWidget;
QT_END_NAMESPACE

namespace Utils {
class FancyMainWindow;
}

namespace Analyzer {
class IAnalyzerTool;
class IAnalyzerEngine;
class AnalyzerStartParameters;

class ANALYZER_EXPORT AnalyzerManager : public QObject
{
    Q_OBJECT

public:
    explicit AnalyzerManager(QObject *parent = 0);
    ~AnalyzerManager();

    static void registerRunControlFactory(ProjectExplorer::IRunControlFactory *factory);
    void extensionsInitialized();
    void shutdown();

    static ProjectExplorer::RunControl *createRunControl
        (ProjectExplorer::RunConfiguration *runConfiguration, const QString &mode);

    // Register a tool and initialize it.
    static void addTool(Analyzer::IAnalyzerTool *tool);
    static IAnalyzerTool *toolById(const QByteArray &id);

    // Dockwidgets are registered to the main window.
    static QDockWidget *createDockWidget(IAnalyzerTool *tool, const QString &title,
        QWidget *widget, Qt::DockWidgetArea area = Qt::TopDockWidgetArea);

    static Utils::FancyMainWindow *mainWindow();

    static void showMode();
    static void selectTool(IAnalyzerTool *tool);
    static void startTool(IAnalyzerTool *tool);
    static void stopTool(IAnalyzerTool *tool);

    // Convienience functions.
    static void startLocalTool(IAnalyzerTool *tool);
    static void startRemoteTool(IAnalyzerTool *tool);

    static QString msgToolStarted(const QString &name);
    static QString msgToolFinished(const QString &name, int issuesFound);

    static void showStatusMessage(const QString &message, int timeoutMS = 10000);
    static void showPermanentStatusMessage(const QString &message);

    static void handleToolFinished(IAnalyzerTool *tool);

private:
    class AnalyzerManagerPrivate;
    friend class AnalyzerManagerPrivate;
    AnalyzerManagerPrivate *const d;
};

} // namespace Analyzer

#endif // ANALYZERMANAGER_H
