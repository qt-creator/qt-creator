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

#include "quicktest_utils.h"
#include "../testframeworkmanager.h"
#include "../testtreeitem.h"

#include <utils/qtcassert.h>

#include <QByteArrayList>

namespace Autotest {
namespace Internal {
namespace QuickTestUtils {

static const QByteArrayList valid = {"QUICK_TEST_MAIN", "QUICK_TEST_OPENGL_MAIN"};

bool isQuickTestMacro(const QByteArray &macro)
{
    return valid.contains(macro);
}

QHash<QString, QString> proFilesForQmlFiles(const Core::Id &id, const QStringList &files)
{
    QHash<QString, QString> result;
    TestTreeItem *rootNode = TestFrameworkManager::instance()->rootNodeForTestFramework(id);
    QTC_ASSERT(rootNode, return result);

    if (files.isEmpty())
        return result;

    for (int row = 0, rootCount = rootNode->childCount(); row < rootCount; ++row) {
        const TestTreeItem *child = rootNode->childItem(row);
        const QString &file = child->filePath();
        if (!file.isEmpty() && files.contains(file)) {
            const QString &proFile = child->proFile();
            if (!proFile.isEmpty())
                result.insert(file, proFile);
        }
        for (int subRow = 0, subCount = child->childCount(); subRow < subCount; ++subRow) {
            const TestTreeItem *grandChild = child->childItem(subRow);
            const QString &file = grandChild->filePath();
            if (!file.isEmpty() && files.contains(file)) {
                const QString &proFile = grandChild->proFile();
                if (!proFile.isEmpty())
                    result.insert(file, proFile);
            }
        }
    }
    return result;
}

} // namespace QuickTestUtils
} // namespace Internal
} // namespace Autotest
