/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
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

#ifndef QMLPROFILERTOOL_H
#define QMLPROFILERTOOL_H

#include <analyzerbase/ianalyzertool.h>
#include <analyzerbase/analyzerruncontrol.h>
#include "qmldebug/qmlprofilereventtypes.h"

QT_BEGIN_NAMESPACE
class QMessageBox;
QT_END_NAMESPACE

namespace QmlProfiler {
namespace Internal {

const char QmlProfilerToolId[] = "QmlProfiler";
const char QmlProfilerLocalActionId[] = "QmlProfiler.Local";
const char QmlProfilerRemoteActionId[] = "QmlProfiler.Remote";

class QmlProfilerTool : public QObject
{
    Q_OBJECT

public:
    explicit QmlProfilerTool(QObject *parent);
    ~QmlProfilerTool();

    Analyzer::AnalyzerRunControl *createRunControl(const Analyzer::AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration = 0);

    QWidget *createWidgets();
    bool prepareTool();
    void startRemoteTool();

    QList <QAction *> profilerContextMenuActions() const;

    // display dialogs / log output
    static QMessageBox *requestMessageBox();
    static void handleHelpRequest(const QString &link);
    static void logState(const QString &msg);
    static void logError(const QString &msg);
    static void showNonmodalWarning(const QString &warningMsg);

public slots:
    void profilerStateChanged();
    void clientRecordingChanged();
    void serverRecordingChanged();
    void clientsDisconnected();
    void setAvailableFeatures(quint64 features);
    void setRecordedFeatures(quint64 features);

    void recordingButtonChanged(bool recording);
    void setRecording(bool recording);

    void gotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber);

private slots:
    void clearData();
    void showErrorDialog(const QString &error);
    void profilerDataModelStateChanged();
    void updateTimeDisplay();
    void showTimeLineSearch();

    void showSaveOption();
    void showLoadOption();
    void showSaveDialog();
    void showLoadDialog();
    void onLoadSaveFinished();

    void toggleRequestedFeature(QAction *action);
    void toggleVisibleFeature(QAction *action);

private:
    void clearDisplay();
    void populateFileFinder(QString projectDirectory = QString(), QString activeSysroot = QString());
    template<QmlDebug::ProfileFeature feature>
    void updateFeatures(quint64 features);
    bool checkForUnsavedNotes();
    void restoreFeatureVisibility();

    class QmlProfilerToolPrivate;
    QmlProfilerToolPrivate *d;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERTOOL_H
