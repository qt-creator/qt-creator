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

#include "itestparser.h"

#include <qmljs/qmljsdocument.h>

#include <QObject>
#include <QMap>
#include <QFutureWatcher>
#include <QTimer>

namespace Core {
class Id;
}

namespace Autotest {
namespace Internal {

class TestCodeParser : public QObject
{
    Q_OBJECT
public:
    enum State {
        Idle,
        PartialParse,
        FullParse,
        Shutdown
    };

    explicit TestCodeParser(TestTreeModel *parent = 0);
    virtual ~TestCodeParser();
    void setState(State state);
    State state() const { return m_parserState; }
    bool isParsing() const { return m_parserState == PartialParse || m_parserState == FullParse; }
    void setDirty() { m_dirty = true; }
    void syncTestFrameworks(const QVector<Core::Id> &frameworkIds);
#ifdef WITH_TESTS
    bool furtherParsingExpected() const
    { return m_singleShotScheduled || m_fullUpdatePostponed || m_partialUpdatePostponed; }
#endif

signals:
    void aboutToPerformFullParse();
    void testParseResultReady(const TestParseResultPtr result);
    void parsingStarted();
    void parsingFinished();
    void parsingFailed();

public:
    void emitUpdateTestTree();
    void updateTestTree();
    void onCppDocumentUpdated(const CPlusPlus::Document::Ptr &document);
    void onQmlDocumentUpdated(const QmlJS::Document::Ptr &document);
    void onStartupProjectChanged(ProjectExplorer::Project *project);
    void onProjectPartsUpdated(ProjectExplorer::Project *project);
    void aboutToShutdown();

private:
    bool postponed(const QStringList &fileList);
    void scanForTests(const QStringList &fileList = QStringList());

    void onDocumentUpdated(const QString &fileName);
    void onTaskStarted(Core::Id type);
    void onAllTasksFinished(Core::Id type);
    void onFinished();
    void onPartialParsingFinished();
    void parsePostponedFiles();
    void releaseParserInternals();

    TestTreeModel *m_model;

    bool m_codeModelParsing = false;
    bool m_fullUpdatePostponed = false;
    bool m_partialUpdatePostponed = false;
    bool m_dirty = false;
    bool m_singleShotScheduled = false;
    bool m_reparseTimerTimedOut = false;
    QSet<QString> m_postponedFiles;
    State m_parserState = Idle;
    QFutureWatcher<TestParseResultPtr> m_futureWatcher;
    QVector<ITestParser *> m_testCodeParsers; // ptrs are still owned by TestFrameworkManager
    QTimer m_reparseTimer;
};

} // namespace Internal
} // namespace Autotest
