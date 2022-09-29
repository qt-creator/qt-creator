// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "squishtesttreemodel.h"

#include <QMap>
#include <QObject>
#include <QString>

namespace Utils { class FilePath; }

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
    void openTestSuite(const Utils::FilePath &suitePath, bool isReopen = false);
    void closeTestSuite(const QString &suiteName);
    void closeAllTestSuites();
    void runTestCase(const QString &suiteName, const QString &testCaseName);
    void runTestSuite(const QString &suiteName);
    void recordTestCase(const QString &suiteName, const QString &testCaseName);
    void addSharedFolder();
    bool removeSharedFolder(const QString &folder);
    void removeAllSharedFolders();
    void openObjectsMap(const QString &suiteName);

signals:
    void testTreeItemCreated(SquishTestTreeItem *item);
    void suiteTreeItemRemoved(const QString &filePath);
    void suiteTreeItemModified(SquishTestTreeItem *item, const QString &displayName);
    void suitesOpened();

private:
    void closeAllInternal();
    void onSessionLoaded();

    QMap<QString, QString> m_suites;
    QStringList m_sharedFolders;

    void modifySuiteItem(const QString &suiteName,
                         const QString &filePath,
                         const QStringList &cases);
};

} // namespace Internal
} // namespace Squish
