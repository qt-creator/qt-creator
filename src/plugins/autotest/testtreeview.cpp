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

#include "autotestconstants.h"
#include "testcodeparser.h"
#include "testrunner.h"
#include "testtreeitem.h"
#include "testtreeitemdelegate.h"
#include "testtreemodel.h"
#include "testtreeview.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <coreplugin/find/itemviewfind.h>

#include <cpptools/cppmodelmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>

#include <qmljstools/qmljsmodelmanager.h>

#include <texteditor/texteditor.h>

#include <QToolButton>
#include <QVBoxLayout>

namespace Autotest {
namespace Internal {

TestTreeView::TestTreeView(QWidget *parent)
    : NavigationTreeView(parent),
      m_context(new Core::IContext(this))
{
    setExpandsOnDoubleClick(false);
    m_context->setWidget(this);
    m_context->setContext(Core::Context(Constants::AUTOTEST_CONTEXT));
    Core::ICore::addContextObject(m_context);
}

void TestTreeView::selectAll()
{
    changeCheckStateAll(Qt::Checked);
}

void TestTreeView::deselectAll()
{
    changeCheckStateAll(Qt::Unchecked);
}

// this avoids the re-evaluation of parent nodes when modifying the child nodes (setData())
void TestTreeView::changeCheckStateAll(const Qt::CheckState checkState)
{
    const TestTreeModel *model = TestTreeModel::instance();

    for (int rootRow = 0; rootRow < model->rowCount(rootIndex()); ++rootRow) {
        QModelIndex currentRootIndex = model->index(rootRow, 0, rootIndex());
        if (!currentRootIndex.isValid())
            return;
        int count = model->rowCount(currentRootIndex);
        QModelIndex last;
        for (int classesRow = 0; classesRow < count; ++classesRow) {
            const QModelIndex classesIndex = model->index(classesRow, 0, currentRootIndex);
            int funcCount = model->rowCount(classesIndex);
            TestTreeItem *item = static_cast<TestTreeItem *>(classesIndex.internalPointer());
            if (item) {
                item->setChecked(checkState);
                if (!item->childCount())
                    last = classesIndex;
            }
            for (int functionRow = 0; functionRow < funcCount; ++functionRow) {
                last = model->index(functionRow, 0, classesIndex);
                TestTreeItem *item = static_cast<TestTreeItem *>(last.internalPointer());
                if (item)
                    item->setChecked(checkState);
            }
        }
        emit dataChanged(currentRootIndex, last);
    }
}

} // namespace Internal
} // namespace Autotest
