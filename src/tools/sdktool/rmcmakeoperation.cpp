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

#include "rmcmakeoperation.h"

#include "addkeysoperation.h"
#include "addcmakeoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include <iostream>

// CMake file stuff:
const char COUNT[] = "CMakeTools.Count";
const char PREFIX[] = "CMakeTools.";

// CMake:
const char ID[] = "Id";

QString RmCMakeOperation::name() const
{
    return QString("rmCMake");
}

QString RmCMakeOperation::helpText() const
{
    return QString("remove a cmake tool from Qt Creator");
}

QString RmCMakeOperation::argumentsHelpText() const
{
    return QString("    --id <ID>  The id of the cmake tool to remove.\n");
}

bool RmCMakeOperation::setArguments(const QStringList &args)
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

int RmCMakeOperation::execute() const
{
    QVariantMap map = load("cmaketools");
    if (map.isEmpty())
        return 0;

    QVariantMap result = rmCMake(map, m_id);
    if (result == map)
        return 2;

    return save(result, "cmaketools") ? 0 : 3;
}

#ifdef WITH_TESTS
bool RmCMakeOperation::test() const
{
    // Add cmakes:
    QVariantMap map = AddCMakeOperation::initializeCMake();
    map = AddCMakeOperation::addCMake(map, "testId", "name", "/tmp/test",
                                      KeyValuePairList({ KeyValuePair("ExtraKey", QVariant("ExtraValue")) }));
    map = AddCMakeOperation::addCMake(map, "testId2", "other name", "/tmp/test2", KeyValuePairList());

    QVariantMap result = rmCMake(QVariantMap(), "nonexistent");
    if (!result.isEmpty())
        return false;

    result = rmCMake(map, "nonexistent");
    if (result != map)
        return false;

    result = rmCMake(map, "testId2");
    if (result == map
            || result.value(COUNT, 0).toInt() != 1
            || !result.contains("CMake.0") || result.value("CMake.0") != map.value("CMake.0"))
        return false;

    result = rmCMake(map, "testId");
    if (result == map
            || result.value(COUNT, 0).toInt() != 1
            || !result.contains("CMake.0") || result.value("CMake.0") != map.value("CMake.1"))
        return false;

    result = rmCMake(result, "testId2");
    if (result == map
            || result.value(COUNT, 0).toInt() != 0)
        return false;

    return true;
}
#endif

QVariantMap RmCMakeOperation::rmCMake(const QVariantMap &map, const QString &id)
{
    // Find count of cmakes:
    bool ok;
    int count = GetOperation::get(map, COUNT).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Count found in cmake tools file seems wrong." << std::endl;
        return map;
    }

    QVariantList cmList;
    for (int i = 0; i < count; ++i) {
        QVariantMap cmData = GetOperation::get(map, QString::fromLatin1(PREFIX) + QString::number(i)).toMap();
        if (cmData.value(ID).toString() != id)
            cmList.append(cmData);
    }

    QVariantMap newMap = AddCMakeOperation::initializeCMake();
    for (int i = 0; i < cmList.count(); ++i)
        newMap.insert(QString::fromLatin1(PREFIX) + QString::number(i), cmList.at(i));
    newMap.insert(COUNT, cmList.count());

    return newMap;
}

