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

#include "rmtoolchainoperation.h"

#include "addkeysoperation.h"
#include "addtoolchainoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include <iostream>

// ToolChain file stuff:
const char COUNT[] = "ToolChain.Count";
const char PREFIX[] = "ToolChain.";

// ToolChain:
const char ID[] = "ProjectExplorer.ToolChain.Id";

QString RmToolChainOperation::name() const
{
    return QString("rmTC");
}

QString RmToolChainOperation::helpText() const
{
    return QString("remove a tool chain");
}

QString RmToolChainOperation::argumentsHelpText() const
{
    return QString("    --id <ID>  The id of the tool chain to remove.\n");
}

bool RmToolChainOperation::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == "--id") {
            if (next.isNull()) {
                std::cerr << "No parameter for --id given." << std::endl << std::endl;
                return false;
            }
            ++i; // skip next;
            m_id = next;
            continue;
        }
    }

    if (m_id.isEmpty())
        std::cerr << "No id given." << std::endl << std::endl;

    return !m_id.isEmpty();
}

int RmToolChainOperation::execute() const
{
    QVariantMap map = load("ToolChains");
    if (map.isEmpty())
        return 0;

    QVariantMap result = rmToolChain(map, m_id);
    if (result == map)
        return 2;

    return save(result, "ToolChains") ? 0 : 3;
}

#ifdef WITH_TESTS
bool RmToolChainOperation::test() const
{
    // Add toolchain:
    QVariantMap map = AddToolChainOperation::initializeToolChains();
    map = AddToolChainOperation::addToolChain(map, "testId", "langId", "name", "/tmp/test", "test-abi",
                                              "test-abi,test-abi2",
                                              KeyValuePairList({KeyValuePair("ExtraKey", QVariant("ExtraValue"))}));
    map = AddToolChainOperation::addToolChain(map, "testId2", "langId", "other name", "/tmp/test2", "test-abi",
                                              "test-abi,test-abi2", KeyValuePairList());

    QVariantMap result = rmToolChain(QVariantMap(), "nonexistent");
    if (!result.isEmpty())
        return false;

    result = rmToolChain(map, "nonexistent");
    if (result != map)
        return false;

    result = rmToolChain(map, "testId2");
    if (result == map
            || result.value(COUNT, 0).toInt() != 1
            || !result.contains("ToolChain.0") || result.value("ToolChain.0") != map.value("ToolChain.0"))
        return false;

    result = rmToolChain(map, "testId");
    if (result == map
            || result.value(COUNT, 0).toInt() != 1
            || !result.contains("ToolChain.0") || result.value("ToolChain.0") != map.value("ToolChain.1"))
        return false;

    result = rmToolChain(result, "testId2");
    if (result == map
            || result.value(COUNT, 0).toInt() != 0)
        return false;

    return true;
}
#endif

QVariantMap RmToolChainOperation::rmToolChain(const QVariantMap &map, const QString &id)
{
    // Find count of tool chains:
    bool ok;
    int count = GetOperation::get(map, COUNT).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Count found in toolchains file seems wrong." << std::endl;
        return map;
    }

    QVariantList tcList;
    for (int i = 0; i < count; ++i) {
        QVariantMap tcData = GetOperation::get(map, QString::fromLatin1(PREFIX) + QString::number(i)).toMap();
        if (tcData.value(ID).toString() != id)
            tcList.append(tcData);
    }

    QVariantMap newMap = AddToolChainOperation::initializeToolChains();
    for (int i = 0; i < tcList.count(); ++i)
        newMap.insert(QString::fromLatin1(PREFIX) + QString::number(i), tcList.at(i));
    newMap.insert(COUNT, tcList.count());

    return newMap;
}

