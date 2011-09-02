/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef RESULTSVIEW_H
#define RESULTSVIEW_H

#include "testsuite.h"
#include "testsettings.h"

#include <coreplugin/ioutputpane.h>

#include <QTableWidget>

class ResultsView;
class TestResultsWindow;
QT_BEGIN_NAMESPACE
class QToolButton;
QT_END_NAMESPACE

ResultsView *testResultsPane();

class ResultsView : public QTableWidget
{
    Q_OBJECT

public:
    explicit ResultsView(QWidget *parent = 0);
    virtual ~ResultsView();

    QString result(int row);
    QString reason(int row);
    QString dataTag(int row);
    QString file(int row);
    QString line(int row);
    int intLine(int row);

    void reselect();

    virtual void clear();
    void setResult(const QString &result, const QString &test, const QString &reason,
        const QString &dataTag, const QString &file, int line);

    void setResultsWindow(TestResultsWindow * = 0);
    TestResultsWindow *resultsWindow() const;
    QStringList failedTests() const { return m_failedTests; }
    void copyResults();

public slots:
    void addScreenshot(const QString &screenshot,
        const QString &testfunction, const QString &file, int line);

signals:
    void defectSelected(TestCaseRec rec);

private slots:
    void onChanged();
    void emitCurSelection();
    void onItemClicked(QTableWidgetItem *);
    void showPassing(bool);
    void showDebugMessages(bool);
    void showSkipped(bool);

private:
    QString formatTestDetails(const QString &test, const QString &dataTag);
    QString formatLocation(const QString &file, const QString &line);
    void append(const QString &res, const QString &test, const QString &reason,
        const QString &dataTag, const QString &file, const QString &line);

    void resize(int width);
    void resizeEvent(QResizeEvent *event);
    void emitSelection(int row);

    QString location(int row);
    void updateScreenshots();
    static QString xmlDequote(const QString &input);
    static QString htmlQuote(const QString &input);
    void updateHidden(const QString &result, bool show);

    int m_lastRow;
    int m_usedRows;
    bool m_ignoreEvent;
    bool m_userLock;
    QMap<QString,QString> m_pendingScreenshots;
    TestCollection m_testCollection;
    TestResultsWindow *m_resultsWindow;
    bool m_showPassing;
    bool m_showDebug;
    bool m_showSkipped;
    QStringList m_failedTests;
    TestSettings m_testSettings;
    const QBrush m_passBrush;
    const QBrush m_failBrush;
    const QBrush m_unexpectedBrush;
};

class TestResultsWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    TestResultsWindow();
    ~TestResultsWindow();

    static TestResultsWindow *instance();

    virtual QString displayName() const { return "Test Results"; }

    QWidget *outputWidget(QWidget *parent);
    QList<QWidget*> toolBarWidgets() const;

    QString name() const;
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);

    bool canFocus();
    bool hasFocus();
    void setFocus();

    virtual bool canNext();
    virtual bool canPrevious();
    virtual void goToNext();
    virtual void goToPrev();
    bool canNavigate();
    ResultsView *resultsView() { return m_resultsView; }

    void addResult(const QString &result, const QString &test, const QString &reason,
        const QString &dataTag, const QString &file, int line);

signals:
    void stopTest();
    void retryFailedTests(const QStringList &);

public slots:
    void onTestStarted();
    void onTestStopped();
    void onTestFinished();
    void retryFailed();
    void copyResults();
    void onSettingsChanged();

private:
    TestSettings m_testSettings;
    ResultsView *m_resultsView;
    QAction *m_stopAction;
    QAction *m_retryAction;
    QAction *m_copyAction;

    QAction *m_showPassingAction;
    QAction *m_showDebugAction;
    QAction *m_showSkipAction;

    QToolButton *m_stopButton;
    QToolButton *m_retryButton;
    QToolButton *m_copyButton;
    QToolButton *m_filterButton;
};

#endif
