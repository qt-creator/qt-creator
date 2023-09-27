// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../itestparser.h"

#include <qmljs/qmljsdocument.h>

#include <QFileSystemWatcher>
#include <QReadWriteLock>

namespace Autotest {
namespace Internal {

class QuickTestParseResult : public TestParseResult
{
public:
    explicit QuickTestParseResult(ITestFramework *framework) : TestParseResult(framework) {}
    TestTreeItem *createTestTreeItem() const override;
};

class QuickTestParser : public QObject, public CppParser
{
    Q_OBJECT
public:
    explicit QuickTestParser(ITestFramework *framework);
    void init(const QSet<Utils::FilePath> &filesToParse, bool fullParse) override;
    void release() override;
    bool processDocument(QPromise<TestParseResultPtr> &promise,
                         const Utils::FilePath &fileName) override;
    Utils::FilePath projectFileForMainCppFile(const Utils::FilePath &fileName);
    QStringList supportedExtensions() const override { return {"qml"}; };

private:
    bool handleQtQuickTest(QPromise<TestParseResultPtr> &promise,
                           CPlusPlus::Document::Ptr document,
                           ITestFramework *framework);
    void handleDirectoryChanged(const QString &directory);
    void doUpdateWatchPaths(const QStringList &directories);
    QString quickTestName(const CPlusPlus::Document::Ptr &doc) const;
    QList<QmlJS::Document::Ptr> scanDirectoryForQuickTestQmlFiles(const Utils::FilePath &srcDir);
    QmlJS::Snapshot m_qmlSnapshot;
    QHash<Utils::FilePath, Utils::FilePath> m_proFilesForQmlFiles;
    QFileSystemWatcher m_directoryWatcher;
    QMap<QString, QMap<QString, QDateTime> > m_watchedFiles;
    QMap<Utils::FilePath, Utils::FilePath> m_mainCppFiles;
    QSet<Utils::FilePath> m_prefilteredFiles;
    QReadWriteLock m_parseLock; // guard for m_mainCppFiles
    bool m_checkForDerivedTests = false;
};

} // namespace Internal
} // namespace Autotest
