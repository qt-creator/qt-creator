/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef TESTCODEPARSER_H
#define TESTCODEPARSER_H

#include "testtreeitem.h"
#include "testtreemodel.h"

#include <cplusplus/CppDocument.h>

#include <qmljs/qmljsdocument.h>

#include <QObject>
#include <QMap>

namespace Core {
class Id;
}

namespace Autotest {
namespace Internal {

struct TestCodeLocationAndType;
class TestInfo;
class UnnamedQuickTestInfo;

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
    void setDirty() { m_dirty = true; }

#ifdef WITH_TESTS
    int autoTestsCount() const;
    int namedQuickTestsCount() const;
    int unnamedQuickTestsCount() const;
#endif

signals:
    void cacheCleared();
    void testItemCreated(const TestTreeItem &item, TestTreeModel::Type type);
    void testItemsCreated(const QList<TestTreeItem> &itemList, TestTreeModel::Type type);
    void testItemModified(TestTreeItem tItem, TestTreeModel::Type type, const QString &file);
    void testItemsRemoved(const QString &filePath, TestTreeModel::Type type);
    void unnamedQuickTestsUpdated(const QString &filePath, const QString &mainFile,
                                  const QMap<QString, TestCodeLocationAndType> &functions);
    void unnamedQuickTestsRemoved(const QString &filePath);
    void parsingStarted();
    void parsingFinished();
    void parsingFailed();
    void partialParsingFinished();

public slots:
    void emitUpdateTestTree();
    void updateTestTree();
    void checkDocumentForTestCode(CPlusPlus::Document::Ptr document);
    void handleQtQuickTest(CPlusPlus::Document::Ptr document);

    void onCppDocumentUpdated(const CPlusPlus::Document::Ptr &document);
    void onQmlDocumentUpdated(const QmlJS::Document::Ptr &document);
    void onStartupProjectChanged(ProjectExplorer::Project *);
    void onProjectPartsUpdated(ProjectExplorer::Project *project);
    void removeFiles(const QStringList &files);

private:
    bool postponed(const QStringList &fileList);
    void scanForTests(const QStringList &fileList = QStringList());
    void clearCache();
    void removeTestsIfNecessary(const QString &fileName);

    void onTaskStarted(Core::Id type);
    void onAllTasksFinished(Core::Id type);
    void onFinished();
    void onPartialParsingFinished();
    void updateUnnamedQuickTests(const QString &fileName, const QString &mainFile,
                                 const QMap<QString, TestCodeLocationAndType> &functions);
    void updateModelAndCppDocMap(CPlusPlus::Document::Ptr document,
                                 const QString &declaringFile, TestTreeItem &testItem);
    void updateModelAndQuickDocMap(QmlJS::Document::Ptr document,
                                   const QString &referencingFile, TestTreeItem &testItem);
    void removeUnnamedQuickTestsByName(const QString &fileName);

    TestTreeModel *m_model;
    QMap<QString, TestInfo> m_cppDocMap;
    QMap<QString, TestInfo> m_quickDocMap;
    QList<UnnamedQuickTestInfo> m_unnamedQuickDocList;
    bool m_codeModelParsing;
    bool m_fullUpdatePostponed;
    bool m_partialUpdatePostponed;
    bool m_dirty;
    bool m_singleShotScheduled;
    QSet<QString> m_postponedFiles;
    State m_parserState;
};

} // namespace Internal
} // Autotest

#endif // TESTCODEPARSER_H
