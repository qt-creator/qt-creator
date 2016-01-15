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

#include "rmdebuggeroperation.h"

#include "adddebuggeroperation.h"
#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <iostream>

// Qt version file stuff:
const char PREFIX[] = "DebuggerItem.";
const char COUNT[] = "DebuggerItem.Count";
#ifdef WITH_TESTS
const char VERSION[] = "Version";
#endif

// Kit:
const char ID[] = "Id";

QString RmDebuggerOperation::name() const
{
    return QLatin1String("rmDebugger");
}

QString RmDebuggerOperation::helpText() const
{
    return QLatin1String("remove a debugger from Qt Creator");
}

QString RmDebuggerOperation::argumentsHelpText() const
{
    return QLatin1String("    --id <ID>                                  id of the debugger to remove.\n");
}

bool RmDebuggerOperation::setArguments(const QStringList &args)
{
    if (args.count() != 2)
        return false;
    if (args.at(0) != QLatin1String("--id"))
        return false;

    m_id = args.at(1);

    if (m_id.isEmpty())
        std::cerr << "No id given." << std::endl << std::endl;

    return !m_id.isEmpty();
}

int RmDebuggerOperation::execute() const
{
    QVariantMap map = load(QLatin1String("Debuggers"));
    if (map.isEmpty())
        map = AddDebuggerOperation::initializeDebuggers();

    QVariantMap result = rmDebugger(map, m_id);

    if (result == map)
        return 2;

    return save(result, QLatin1String("Debuggers")) ? 0 : 3;
}

#ifdef WITH_TESTS
bool RmDebuggerOperation::test() const
{

    QVariantMap map =
            AddDebuggerOperation::addDebugger(AddDebuggerOperation::initializeDebuggers(),
                                              QLatin1String("id1"), QLatin1String("Name1"),
                                              2, QLatin1String("/tmp/debugger1"),
                                              QStringList() << QLatin1String("test11") << QLatin1String("test12"),
                                              KeyValuePairList());
    map =
            AddDebuggerOperation::addDebugger(map, QLatin1String("id2"), QLatin1String("Name2"),
                                              2, QLatin1String("/tmp/debugger2"),
                                              QStringList() << QLatin1String("test21") << QLatin1String("test22"),
                                              KeyValuePairList());

    QVariantMap result = rmDebugger(map, QLatin1String("id2"));
    if (result.count() != 3
            || !result.contains(QLatin1String("DebuggerItem.0"))
            || !result.contains(QLatin1String(COUNT))
            || result.value(QLatin1String(COUNT)).toInt() != 1
            || !result.contains(QLatin1String(VERSION))
            || result.value(QLatin1String(VERSION)).toInt() != 1)
        return false;

    result = rmDebugger(map, QLatin1String("unknown"));
    if (result != map)
        return false;

    result = rmDebugger(map, QLatin1String("id2"));
    if (result.count() != 3
            || !result.contains(QLatin1String("DebuggerItem.0"))
            || !result.contains(QLatin1String(COUNT))
            || result.value(QLatin1String(COUNT)).toInt() != 1
            || !result.contains(QLatin1String(VERSION))
            || result.value(QLatin1String(VERSION)).toInt() != 1)
        return false;

    result = rmDebugger(result, QLatin1String("id1"));
    if (result.count() != 2
            || !result.contains(QLatin1String(COUNT))
            || result.value(QLatin1String(COUNT)).toInt() != 0
            || !result.contains(QLatin1String(VERSION))
            || result.value(QLatin1String(VERSION)).toInt() != 1)
        return false;

    return true;
}
#endif

QVariantMap RmDebuggerOperation::rmDebugger(const QVariantMap &map, const QString &id)
{
    QVariantMap result = AddDebuggerOperation::initializeDebuggers();

    QVariantList debuggerList;
    bool ok;
    int count = GetOperation::get(map, QLatin1String(COUNT)).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: The count found in map is not an integer." << std::endl;
        return map;
    }

    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(PREFIX) + QString::number(i);
        QVariantMap debugger = map.value(key).toMap();
        if (debugger.value(QLatin1String(ID)).toString() == id)
            continue;
        debuggerList << debugger;
    }
    if (debuggerList.count() == map.count() - 2) {
        std::cerr << "Error: Id was not found." << std::endl;
        return map;
    }

    // remove data:
    QStringList toRemove;
    toRemove << QLatin1String(COUNT);
    result = RmKeysOperation::rmKeys(result, toRemove);

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(QLatin1String(COUNT), count - 1);

    for (int i = 0; i < debuggerList.count(); ++i) {
        data << KeyValuePair(QStringList() << QString::fromLatin1(PREFIX) + QString::number(i),
                             debuggerList.at(i));
    }

    return AddKeysOperation::addKeys(result, data);
}

