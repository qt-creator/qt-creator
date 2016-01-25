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
class UnnamedQuickTestInfo;
class GTestInfo;
struct GTestCaseSpec;

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
    int gtestNamesAndSetsCount() const;
#endif

signals:
    void cacheCleared();
    void testItemCreated(TestTreeItem *item, TestTreeModel::Type type);
    void testItemModified(TestTreeItem *tItem, TestTreeModel::Type type, const QStringList &file);
    void testItemsRemoved(const QString &filePath, TestTreeModel::Type type);
    void unnamedQuickTestsUpdated(const QString &mainFile,
                                  const QMap<QString, TestCodeLocationAndType> &functions);
    void unnamedQuickTestsRemoved(const QString &filePath);
    void gTestsRemoved(const QString &filePath);
    void parsingStarted();
    void parsingFinished();
    void parsingFailed();
    void partialParsingFinished();

public slots:
    void emitUpdateTestTree();
    void updateTestTree();
    void checkDocumentForTestCode(CPlusPlus::Document::Ptr document);
    void handleQtQuickTest(CPlusPlus::Document::Ptr document);
    void handleGTest(const QString &filePath);

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
                                 const QString &declaringFile, TestTreeItem *testItem);
    void updateModelAndQuickDocMap(QmlJS::Document::Ptr document,
                                   const QString &referencingFile, TestTreeItem *testItem);
    void updateGTests(const CPlusPlus::Document::Ptr &doc,
                      const QMap<GTestCaseSpec, TestCodeLocationList> &tests);
    void removeUnnamedQuickTestsByName(const QString &fileName);
    void removeGTestsByName(const QString &fileName);

    TestTreeModel *m_model;

    // FIXME remove me again
    struct ReferencingInfo
    {
        QString referencingFile;
        TestTreeModel::Type type;
    };

    QMap<QString, ReferencingInfo> m_referencingFiles;
    QList<UnnamedQuickTestInfo> m_unnamedQuickDocList;
    QList<GTestInfo> m_gtestDocList;
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
