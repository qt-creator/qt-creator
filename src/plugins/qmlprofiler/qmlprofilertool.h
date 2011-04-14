/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLPROFILERTOOL_H
#define QMLPROFILERTOOL_H

#include <analyzerbase/ianalyzertool.h>
#include <analyzerbase/ianalyzerengine.h>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerTool : public Analyzer::IAnalyzerTool
{
    Q_OBJECT
public:
    explicit QmlProfilerTool(QObject *parent = 0);
    ~QmlProfilerTool();

    QString id() const;
    QString displayName() const;
    ToolMode mode() const;

    void initialize(ExtensionSystem::IPlugin *plugin);
    void extensionsInitialized();

    Analyzer::IAnalyzerEngine *createEngine(const Analyzer::AnalyzerStartParameters &sp,
                                            ProjectExplorer::RunConfiguration *runConfiguration = 0);

    Analyzer::IAnalyzerOutputPaneAdapter *outputPaneAdapter();
    QWidget *createToolBarWidget();
    QWidget *createTimeLineWidget();

    bool canRunRemotely() const;

    void clearDisplay();

public slots:
    void connectClient();
    void disconnectClient();

    void startRecording();
    void stopRecording();
    void setRecording(bool recording);

    void gotoSourceLocation(const QString &fileUrl, int lineNumber);
    void updateTimer(qreal elapsedSeconds);

signals:
    void setTimeLabel(const QString &);
    void fetchingData(bool);
    void connectionFailed();

private slots:
    void updateProjectFileList();
    void attach();
    void updateAttachAction();
    void tryToConnect();
    void connectionStateChanged();

private:
    void connectToClient();
    void resetWindow();

    class QmlProfilerToolPrivate;
    QmlProfilerToolPrivate *d;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERTOOL_H
