/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANALYZERMANAGER_H
#define ANALYZERMANAGER_H

#include "analyzerbase_global.h"

#include <coreplugin/id.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QDockWidget;
class QAction;
QT_END_NAMESPACE

namespace Utils { class FancyMainWindow; }
namespace ProjectExplorer { class RunConfiguration; }

namespace Analyzer {

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

    static AnalyzerRunControl *createRunControl(const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration);
};

} // namespace Analyzer

#endif // ANALYZERMANAGER_H
