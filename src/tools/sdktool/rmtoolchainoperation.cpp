/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "rmtoolchainoperation.h"

#include "addkeysoperation.h"
#include "addtoolchainoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include <iostream>

// ToolChain file stuff:
static char COUNT[] = "ToolChain.Count";
static char PREFIX[] = "ToolChain.";

// ToolChain:
static char ID[] = "ProjectExplorer.ToolChain.Id";

QString RmToolChainOperation::name() const
{
    return QLatin1String("rmTC");
}

QString RmToolChainOperation::helpText() const
{
    return QLatin1String("remove a tool chain from Qt Creator");
}

QString RmToolChainOperation::argumentsHelpText() const
{
    return QLatin1String("    --id <ID>  The id of the tool chain to remove.\n");
}

bool RmToolChainOperation::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == QLatin1String("--id")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_id = next;
            continue;
        }
    }

    return !m_id.isEmpty();
}

int RmToolChainOperation::execute() const
{
    QVariantMap map = load(QLatin1String("toolchains"));
    if (map.isEmpty())
        return 0;

    QVariantMap result = rmToolChain(map, m_id);
    if (result == map)
        return -2;

    return save(map, QLatin1String("toolchains")) ? 0 : -3;
}

#ifdef WITH_TESTS
bool RmToolChainOperation::test() const
{
    // Add toolchain:
    QVariantMap map = AddToolChainOperation::initializeToolChains();
    map = AddToolChainOperation::addToolChain(map, QLatin1String("testId"), QLatin1String("name"), QLatin1String("/tmp/test"),
                                              QLatin1String("test-abi"), QLatin1String("test-abi,test-abi2"),
                                              KeyValuePairList() << KeyValuePair(QLatin1String("ExtraKey"), QVariant(QLatin1String("ExtraValue"))));
    map = AddToolChainOperation::addToolChain(map, QLatin1String("testId2"), QLatin1String("other name"), QLatin1String("/tmp/test2"),
                                              QLatin1String("test-abi"), QLatin1String("test-abi,test-abi2"),
                                              KeyValuePairList());

    QVariantMap result = rmToolChain(QVariantMap(), QLatin1String("nonexistant"));
    if (!result.isEmpty())
        return false;

    result = rmToolChain(map, QLatin1String("nonexistant"));
    if (result != map)
        return false;

    result = rmToolChain(map, QLatin1String("testId2"));
    if (result == map
            || result.value(QLatin1String(COUNT), 0).toInt() != 1
            || !result.contains(QLatin1String("ToolChain.0"))
            || result.value(QLatin1String("ToolChain.0")) != map.value(QLatin1String("ToolChain.0")))
        return false;

    result = rmToolChain(map, QLatin1String("testId"));
    if (result == map
            || result.value(QLatin1String(COUNT), 0).toInt() != 1
            || !result.contains(QLatin1String("ToolChain.0"))
            || result.value(QLatin1String("ToolChain.0")) != map.value(QLatin1String("ToolChain.1")))
        return false;

    result = rmToolChain(result, QLatin1String("testId2"));
    if (result == map
            || result.value(QLatin1String(COUNT), 0).toInt() != 0)
        return false;

    return true;
}
#endif

QVariantMap RmToolChainOperation::rmToolChain(const QVariantMap &map, const QString &id)
{
    // Find count of tool chains:
    bool ok;
    int count = GetOperation::get(map, QLatin1String(COUNT)).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Count found in toolchains file seems wrong." << std::endl;
        return map;
    }

    QVariantList tcList;
    for (int i = 0; i < count; ++i) {
        QVariantMap tcData = GetOperation::get(map, QString::fromLatin1(PREFIX) + QString::number(i)).toMap();
        if (tcData.value(QLatin1String(ID)).toString() != id)
            tcList.append(tcData);
    }

    QVariantMap newMap = AddToolChainOperation::initializeToolChains();
    for (int i = 0; i < tcList.count(); ++i)
        newMap.insert(QString::fromLatin1(PREFIX) + QString::number(i), tcList.at(i));
    newMap.insert(QLatin1String(COUNT), tcList.count());

    return newMap;
}

