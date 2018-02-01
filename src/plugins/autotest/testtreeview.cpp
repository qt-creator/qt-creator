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
#include "testtreeitem.h"
#include "testtreemodel.h"
#include "testtreeview.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

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

static void changeCheckStateAll(const Qt::CheckState checkState)
{
    TestTreeModel *model = TestTreeModel::instance();
    for (int row = 0, count = model->rowCount(); row < count; ++row)
        model->setData(model->index(row, 0), checkState, Qt::CheckStateRole);
}

void TestTreeView::selectAll()
{
    changeCheckStateAll(Qt::Checked);
}

void TestTreeView::deselectAll()
{
    changeCheckStateAll(Qt::Unchecked);
}

} // namespace Internal
} // namespace Autotest
