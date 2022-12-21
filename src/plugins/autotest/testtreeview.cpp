// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testtreeview.h"

#include "autotestconstants.h"
#include "testtreemodel.h"

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
