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

#include "testtreeitem.h"
#include "testtreemodel.h"

#include <cplusplus/CppDocument.h>

#include <qmljs/qmljsdocument.h>

#include <QObject>
#include <QMap>
#include <QFutureWatcher>

namespace Core {
class Id;
}

namespace CppTools {
class CppModelManager;
}

namespace Autotest {
namespace Internal {

class TestParseResult
{
public:
    explicit TestParseResult(TestTreeModel::Type t = TestTreeModel::Invalid) : type(t) {}
    virtual ~TestParseResult() { qDeleteAll(children); }

    virtual TestTreeItem *createTestTreeItem() const = 0;

    QVector<TestParseResult *> children;
    TestTreeModel::Type type;
    TestTreeItem::Type itemType = TestTreeItem::Root;
    QString displayName;
    QString fileName;
    QString proFile;
    QString name;
    unsigned line = 0;
    unsigned column = 0;
};

class QtTestParseResult : public TestParseResult
{
public:
    explicit QtTestParseResult() : TestParseResult(TestTreeModel::AutoTest) {}
    TestTreeItem *createTestTreeItem() const override;
};

class QuickTestParseResult : public TestParseResult
{
public:
    explicit QuickTestParseResult() : TestParseResult(TestTreeModel::QuickTest) {}
    TestTreeItem *createTestTreeItem() const override;
};

class GoogleTestParseResult : public TestParseResult
{
public:
    explicit GoogleTestParseResult() : TestParseResult(TestTreeModel::GoogleTest) {}
    TestTreeItem *createTestTreeItem() const override;
    bool parameterized = false;
    bool typed = false;
    bool disabled = false;
};

using TestParseResultPtr = QSharedPointer<TestParseResult>;

class ITestParser
{
public:
    virtual ~ITestParser() { }
    virtual void init(const QStringList &filesToParse) = 0;
    virtual bool processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                                 const QString &fileName) = 0;
};

class CppParser : public ITestParser
{
public:
    void init(const QStringList &filesToParse) override;
    static bool selectedForBuilding(const QString &fileName);

protected:
    CPlusPlus::Snapshot m_cppSnapshot;
};

class QtTestParser : public CppParser
{
public:
    void init(const QStringList &filesToParse) override;
    bool processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                         const QString &fileName) override;

private:
    QHash<QString, QString> m_testCaseNames;
};

class QuickTestParser : public CppParser
{
public:
    void init(const QStringList &filesToParse) override;
    bool processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                         const QString &fileName) override;
private:
    QmlJS::Snapshot m_qmlSnapshot;
};

class GoogleTestParser : public CppParser
{
public:
    bool processDocument(QFutureInterface<TestParseResultPtr> futureInterface,
                         const QString &fileName) override;
};

class TestCodeParser : public QObject
{
    Q_OBJECT
public:
    enum State {
        Idle,
        PartialParse,
        FullParse,
        Disabled
    };

    explicit TestCodeParser(TestTreeModel *parent = 0);
    virtual ~TestCodeParser();
    void setState(State state);
    State state() const { return m_parserState; }
    bool isParsing() const { return m_parserState == PartialParse || m_parserState == FullParse; }
    void setDirty() { m_dirty = true; }
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

public slots:
    void emitUpdateTestTree();
    void updateTestTree();
    void onCppDocumentUpdated(const CPlusPlus::Document::Ptr &document);
    void onQmlDocumentUpdated(const QmlJS::Document::Ptr &document);
    void onStartupProjectChanged(ProjectExplorer::Project *project);
    void onProjectPartsUpdated(ProjectExplorer::Project *project);

private:
    bool postponed(const QStringList &fileList);
    void scanForTests(const QStringList &fileList = QStringList());

    void onTaskStarted(Core::Id type);
    void onAllTasksFinished(Core::Id type);
    void onFinished();
    void onPartialParsingFinished();

    TestTreeModel *m_model;

    bool m_codeModelParsing;
    bool m_fullUpdatePostponed;
    bool m_partialUpdatePostponed;
    bool m_dirty;
    bool m_singleShotScheduled;
    QSet<QString> m_postponedFiles;
    State m_parserState;
    QFutureWatcher<TestParseResultPtr> m_futureWatcher;
    QVector<ITestParser *> m_testCodeParsers;
};

} // namespace Internal
} // Autotest

Q_DECLARE_METATYPE(Autotest::Internal::TestParseResultPtr)
