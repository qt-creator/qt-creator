// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestparser.h"

#include <qmljs/qmljsdocument.h>

#include <utils/filesystemwatcher.h>

#include <QReadWriteLock>

namespace Autotest::Internal {

class QuickTestParseResult : public TestParseResult
{
public:
    explicit QuickTestParseResult(ITestFramework *framework) : TestParseResult(framework) {}
    TestTreeItem *createTestTreeItem() const override;
};

class QuickTestParseContext : public CppParseContext
{
public:
    QmlJS::Snapshot qmlSnapshot;
    QHash<Utils::FilePath, Utils::FilePath> proFilesForQmlFiles;
    QSet<Utils::FilePath> prefilteredFiles;
    bool checkForDerivedTests = false;
};

class QuickTestParser : public QObject, public CppParser
{
    Q_OBJECT
public:
    explicit QuickTestParser(ITestFramework *framework);
    DocumentProcessor init(const QSet<Utils::FilePath> &filesToParse, bool fullParse) override;
    Utils::FilePath projectFileForMainCppFile(const Utils::FilePath &fileName);
    QStringList supportedExtensions() const override { return {"qml"}; };

private:
    bool processDocument(QPromise<TestParseResultPtr> &promise,
                         const QuickTestParseContext &context,
                         const Utils::FilePath &fileName);
    bool handleQtQuickTest(QPromise<TestParseResultPtr> &promise,
                           const QuickTestParseContext &context,
                           CPlusPlus::Document::Ptr document,
                           ITestFramework *framework);
    void handleDirectoryChanged(const Utils::FilePath &directory);
    void doUpdateWatchPaths(const Utils::FilePaths &directories);
    QString quickTestName(const QuickTestParseContext &context,
                          const CPlusPlus::Document::Ptr &doc) const;
    QList<QmlJS::Document::Ptr> scanDirectoryForQuickTestQmlFiles(const Utils::FilePath &srcDir);

    Utils::FileSystemWatcher m_directoryWatcher;
    QMap<Utils::FilePath, QMap<QString, QDateTime> > m_watchedFiles;
    QMap<Utils::FilePath, Utils::FilePath> m_mainCppFiles;
    QReadWriteLock m_parseLock; // guard for m_mainCppFiles
};

} // namespace Autotest::Internal
