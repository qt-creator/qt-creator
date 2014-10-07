/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
**
****************************************************************************/

#include "testvisitor.h"

#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Symbols.h>

#include <QList>

namespace Autotest {
namespace Internal {

TestVisitor::TestVisitor(const QString &fullQualifiedClassName)
    : m_className(fullQualifiedClassName)
{
}

TestVisitor::~TestVisitor()
{
}

static QList<QString> ignoredFunctions = QList<QString>() << QLatin1String("initTestCase")
                                                          << QLatin1String("cleanupTestCase")
                                                          << QLatin1String("init")
                                                          << QLatin1String("cleanup");

bool TestVisitor::visit(CPlusPlus::Class *symbol)
{
    const CPlusPlus::Overview o;
    CPlusPlus::LookupContext lc;

    unsigned count = symbol->memberCount();
    for (unsigned i = 0; i < count; ++i) {
        CPlusPlus::Symbol *member = symbol->memberAt(i);
        CPlusPlus::Type *type = member->type().type();

        const QString className = o.prettyName(lc.fullyQualifiedName(member->enclosingClass()));
        if (className != m_className)
            continue;

        if (auto func = type->asFunctionType()) {
            if (func->isSlot() && member->isPrivate()) {
                QString name = o.prettyName(func->name());
                if (!ignoredFunctions.contains(name) && !name.endsWith(QLatin1String("_data"))) {
                    TestCodeLocation location;
                    location.m_fileName = QLatin1String(member->fileName());
                    location.m_line = member->line();
                    location.m_column = member->column() - 1;
                    m_privSlots.insert(name, location);
                }
            }
        }
    }
    return true;
}

} // namespace Internal
} // namespace Autotest
