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

#include "qttest_utils.h"
#include "qttesttreeitem.h"
#include "../testframeworkmanager.h"

#include <utils/qtcassert.h>

#include <QByteArrayList>

namespace Autotest {
namespace Internal {
namespace QTestUtils {

static const QByteArrayList valid = {"QTEST_MAIN", "QTEST_APPLESS_MAIN", "QTEST_GUILESS_MAIN"};

bool isQTestMacro(const QByteArray &macro)
{
    return valid.contains(macro);
}

QHash<QString, QString> testCaseNamesForFiles(const Core::Id &id, const QStringList &files)
{
    QHash<QString, QString> result;
    TestTreeItem *rootNode = TestFrameworkManager::instance()->rootNodeForTestFramework(id);
    QTC_ASSERT(rootNode, return result);

    for (int row = 0, rootCount = rootNode->childCount(); row < rootCount; ++row) {
        const TestTreeItem *child = rootNode->childItem(row);
        if (files.contains(child->filePath())) {
            result.insert(child->filePath(), child->name());
        }
        for (int childRow = 0, count = child->childCount(); childRow < count; ++childRow) {
            const TestTreeItem *grandChild = child->childItem(childRow);
            if (files.contains(grandChild->filePath()))
                result.insert(grandChild->filePath(), child->name());
        }
    }
    return result;
}

QMultiHash<QString, QString> alternativeFiles(const Core::Id &id, const QStringList &files)
{
    QMultiHash<QString, QString> result;
    TestTreeItem *rootNode = TestFrameworkManager::instance()->rootNodeForTestFramework(id);
    QTC_ASSERT(rootNode, return result);

    for (int row = 0, rootCount = rootNode->childCount(); row < rootCount; ++row) {
        const TestTreeItem *child = rootNode->childItem(row);
        const QString &baseFilePath = child->filePath();
        for (int childRow = 0, count = child->childCount(); childRow < count; ++childRow) {
            auto grandChild = static_cast<const QtTestTreeItem *>(child->childItem(childRow));
            const QString &filePath = grandChild->filePath();
            if (grandChild->inherited() && baseFilePath != filePath && files.contains(filePath)) {
                if (!result.contains(filePath, baseFilePath))
                    result.insert(filePath, baseFilePath);
            }
        }
    }
    return result;
}

} // namespace QTestUtils
} // namespace Internal
} // namespace Autotest
