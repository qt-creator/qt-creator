// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "squishtesttreemodel.h"

#include <utils/filepath.h>

#include <QMap>
#include <QObject>
#include <QString>

namespace Squish {
namespace Internal {

class SquishFileHandler : public QObject
{
    Q_OBJECT
public:
    explicit SquishFileHandler(QObject *parent = nullptr);
    ~SquishFileHandler() override = default;
    static SquishFileHandler *instance();
    void openTestSuites();
    void openTestSuite(const Utils::FilePath &suiteConfPath, bool isReopen = false);
    void closeTestSuite(const QString &suiteName);
    void closeAllTestSuites();
    void deleteTestCase(const QString &suiteName, const QString &testCaseName);
    void runTestCase(const QString &suiteName, const QString &testCaseName);
    void runTestSuite(const QString &suiteName);
    void recordTestCase(const QString &suiteName, const QString &testCaseName);
    void addSharedFolder();
    void setSharedFolders(const Utils::FilePaths &folders);
    bool removeSharedFolder(const Utils::FilePath &folder);
    void removeAllSharedFolders();
    void openObjectsMap(const QString &suiteName);

signals:
    void clearedSharedFolders();
    void testTreeItemCreated(SquishTestTreeItem *item);
    void suiteTreeItemRemoved(const QString &suiteName);
    void suiteTreeItemModified(SquishTestTreeItem *item, const QString &displayName);
    void testCaseRemoved(const QString &suiteName, const QString &testCaseName);
    void suitesOpened();

private:
    void closeAllInternal();
    void onSessionLoaded();
    void updateSquishServerGlobalScripts();
    QStringList suitePathsAsStringList() const;

    void modifySuiteItem(const QString &suiteName,
                         const Utils::FilePath &suiteConf,
                         const QStringList &cases);

    QMap<QString, Utils::FilePath> m_suites;
    Utils::FilePaths m_sharedFolders;
};

} // namespace Internal
} // namespace Squish
