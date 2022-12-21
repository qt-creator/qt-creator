// Copyright (C) 2019 Jochen Seemann
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "catchframework.h"

#include "catchtestparser.h"
#include "catchtreeitem.h"
#include "../autotesttr.h"

namespace Autotest {
namespace Internal {

const char *CatchFramework::name() const
{
    return "Catch";
}

QString CatchFramework::displayName() const
{
    return Tr::tr("Catch Test");
}

unsigned CatchFramework::priority() const
{
    return 12;
}

ITestParser *CatchFramework::createTestParser()
{
    return new CatchTestParser(this);
}

ITestTreeItem *CatchFramework::createRootNode()
{
    return new CatchTreeItem(this,
                             displayName(),
                             Utils::FilePath(), ITestTreeItem::Root);
}

} // namespace Internal
} // namespace Autotest
