/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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

#include "squishtesttreemodel.h"

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
    void closeTestSuite(const QString &suiteName);
    void closeAllTestSuites();
    void runTestCase(const QString &suiteName, const QString &testCaseName);
    void runTestSuite(const QString &suiteName);
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
    QMap<QString, QString> m_suites;
    QStringList m_sharedFolders;

    void modifySuiteItem(const QString &suiteName,
                         const QString &filePath,
                         const QStringList &cases);
};

} // namespace Internal
} // namespace Squish
