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
class TestTreeModel;
class TestTreeItem;

class TestCodeParser : public QObject
{
    Q_OBJECT
public:
    explicit TestCodeParser(TestTreeModel *parent = 0);
    virtual ~TestCodeParser();

signals:

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
    void scanForTests(const QStringList &fileList = QStringList());
    void clearMaps();
    void removeTestsIfNecessary(const QString &fileName);
    void removeTestsIfNecessaryByProFile(const QString &proFile);
    void removeUnnamedQuickTests(const QString &fileName, const QStringList &testFunctions);
    void recreateUnnamedQuickTest(const QMap<QString, TestCodeLocationAndType> &testFunctions,
                                  const QString &mainFile, TestTreeItem *rootItem);
    void onTaskStarted(Core::Id type);
    void onAllTasksFinished(Core::Id type);
    void updateModelAndCppDocMap(CPlusPlus::Document::Ptr document, const QString &declFileName,
                                 TestTreeItem *testItem, TestTreeItem *rootItem);
    void updateModelAndQuickDocMap(QmlJS::Document::Ptr qmlDoc, const QString &currentQmlJSFile,
                                   const QString &referencingFileName,
                                   TestTreeItem *testItem, TestTreeItem *rootItem);

    TestTreeModel *m_model;
    QMap<QString, TestInfo> m_cppDocMap;
    QMap<QString, TestInfo> m_quickDocMap;
    bool m_parserEnabled;
    bool m_pendingUpdate;
};

} // namespace Internal
} // Autotest

#endif // TESTCODEPARSER_H
