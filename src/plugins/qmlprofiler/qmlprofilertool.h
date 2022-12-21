// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofiler_global.h"

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

    static QmlProfilerTool *instance();

    void finalizeRunControl(QmlProfilerRunner *runWorker);

    bool prepareTool();
    ProjectExplorer::RunControl *attachToWaitingApplication();

    static QList <QAction *> profilerContextMenuActions();

    // display dialogs / log output
    static void logState(const QString &msg);
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
