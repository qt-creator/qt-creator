/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
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
    void parsingFinished();
    void partialParsingFinished();

public slots:
    void emitUpdateTestTree();
    void updateTestTree();
    void checkDocumentForTestCode(CPlusPlus::Document::Ptr document);
    void handleQtQuickTest(CPlusPlus::Document::Ptr document);

    void onCppDocumentUpdated(const CPlusPlus::Document::Ptr &document);
    void onQmlDocumentUpdated(const QmlJS::Document::Ptr &document);
    void removeFiles(const QStringList &files);
    void onProFileEvaluated();

private:
    bool postponed(const QStringList &fileList);
    void scanForTests(const QStringList &fileList = QStringList());
    void clearMaps();
    void removeTestsIfNecessary(const QString &fileName);
    void removeTestsIfNecessaryByProFile(const QString &proFile);

    void onTaskStarted(Core::Id type);
    void onAllTasksFinished(Core::Id type);
    void onPartialParsingFinished();
    void updateUnnamedQuickTests(const QString &fileName, const QString &mainFile,
                                 const QMap<QString, TestCodeLocationAndType> &functions);
    void updateModelAndCppDocMap(CPlusPlus::Document::Ptr document,
                                 const QString &declaringFile, TestTreeItem &testItem);
    void updateModelAndQuickDocMap(QmlJS::Document::Ptr document,
                                   const QString &referencingFile, TestTreeItem &testItem);

    TestTreeModel *m_model;
    QMap<QString, TestInfo> m_cppDocMap;
    QMap<QString, TestInfo> m_quickDocMap;
    bool m_parserEnabled;
    bool m_pendingUpdate;
    bool m_fullUpdatePostPoned;
    bool m_partialUpdatePostPoned;
    QSet<QString> m_postPonedFiles;
    State m_parserState;
};

} // namespace Internal
} // Autotest

#endif // TESTCODEPARSER_H
