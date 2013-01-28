/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#ifndef QMLPROFILERTOOL_H
#define QMLPROFILERTOOL_H

#include <analyzerbase/ianalyzertool.h>
#include <analyzerbase/ianalyzerengine.h>

QT_BEGIN_NAMESPACE
class QMessageBox;
QT_END_NAMESPACE

namespace QmlProfiler {
namespace Internal {

class QmlProfilerTool : public Analyzer::IAnalyzerTool
{
    Q_OBJECT

public:
    explicit QmlProfilerTool(QObject *parent);
    ~QmlProfilerTool();

    Core::Id id() const;
    ProjectExplorer::RunMode runMode() const;
    QString displayName() const;
    QString description() const;
    ToolMode toolMode() const;

    void extensionsInitialized() {}

    Analyzer::IAnalyzerEngine *createEngine(const Analyzer::AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration = 0);

    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                ProjectExplorer::RunMode mode) const;

    Analyzer::AnalyzerStartParameters createStartParameters(
            ProjectExplorer::RunConfiguration *runConfiguration,
            ProjectExplorer::RunMode mode) const;

    QWidget *createWidgets();
    void startTool(Analyzer::StartMode mode);

    QList <QAction *> profilerContextMenuActions() const;

    // display dialogs / log output
    static QMessageBox *requestMessageBox();
    static void handleHelpRequest(const QString &link);
    static void logStatus(const QString &msg);
    static void logError(const QString &msg);
    static void showNonmodalWarning(const QString &warningMsg);

public slots:
    void profilerStateChanged();
    void clientRecordingChanged();
    void serverRecordingChanged();
    void clientsDisconnected();

    void recordingButtonChanged(bool recording);
    void setRecording(bool recording);

    void gotoSourceLocation(const QString &fileUrl, int lineNumber, int columnNumber);

private slots:
    void clearData();
    void showErrorDialog(const QString &error);
    void profilerDataModelStateChanged();
    void updateTimeDisplay();

    void showSaveOption();
    void showSaveDialog();
    void showLoadDialog();

private:
    void clearDisplay();
    void populateFileFinder(QString projectDirectory = QString(), QString activeSysroot = QString());

    class QmlProfilerToolPrivate;
    QmlProfilerToolPrivate *d;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERTOOL_H
