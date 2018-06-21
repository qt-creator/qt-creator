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

#include "qmlprofiler_global.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilereventtypes.h"

#include <QAction>
#include <QObject>

namespace ProjectExplorer { class RunControl; }

namespace QmlProfiler {

class QmlProfilerModelManager;
class QmlProfilerStateManager;

namespace Internal {

class QmlProfilerRunner;
class QmlProfilerClientManager;

class QMLPROFILER_EXPORT QmlProfilerTool : public QObject
{
    Q_OBJECT

public:
    QmlProfilerTool();
    ~QmlProfilerTool() override;

    void finalizeRunControl(QmlProfilerRunner *runWorker);

    bool prepareTool();
    ProjectExplorer::RunControl *attachToWaitingApplication();

    static QList <QAction *> profilerContextMenuActions();

    // display dialogs / log output
    static void logState(const QString &msg);
    static void logError(const QString &msg);
    static void showNonmodalWarning(const QString &warningMsg);

    QmlProfilerClientManager *clientManager();
    QmlProfilerModelManager *modelManager();
    QmlProfilerStateManager *stateManager();

    void profilerStateChanged();
    void serverRecordingChanged();
    void clientsDisconnected();
    void setAvailableFeatures(quint64 features);
    void setRecordedFeatures(quint64 features);
    void recordingButtonChanged(bool recording);

    void gotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber);

    void showSaveDialog();
    void showLoadDialog();

    void profileStartupProject();

    QAction *startAction() const;
    QAction *stopAction() const;

private:
    void clearEvents();
    void clearData();
    void showErrorDialog(const QString &error);
    void profilerDataModelStateChanged();
    void updateTimeDisplay();
    void showTimeLineSearch();

    void onLoadSaveFinished();

    void toggleRequestedFeature(QAction *action);
    void toggleVisibleFeature(QAction *action);

    void updateRunActions();
    void clearDisplay();
    template<ProfileFeature feature>
    void updateFeatures(quint64 features);
    bool checkForUnsavedNotes();
    void setButtonsEnabled(bool enable);
    void createInitialTextMarks();

    void initialize();
    void finalize();
    void clear();

    class QmlProfilerToolPrivate;
    QmlProfilerToolPrivate *d;
};

} // namespace Internal
} // namespace QmlProfiler
