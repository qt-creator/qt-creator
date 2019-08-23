/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Copyright (C) 2019 Christoph Schlosser, B/S/H/ <christoph.schlosser@bshg.com>
**
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

#include "addvalueoperation.h"
#include "addkeysoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include <iomanip>
#include <iostream>

namespace {
constexpr auto SUCCESS = 0;
constexpr auto FAILURE = !SUCCESS;
} // namespace

QString AddValueOperation::name() const
{
    return QLatin1String("addValue");
}

QString AddValueOperation::helpText() const
{
    return QLatin1String("add a value to an existing list");
}

QString AddValueOperation::argumentsHelpText() const
{
    return QLatin1String(
        "A file (relative to top-level settings directory and without .xml extension)\n"
        "followed by the key of the list followed by one or more type:values to append to the "
        "list.\n");
}

bool AddValueOperation::setArguments(const QStringList &args)
{
    if (args.size() < 3) {
        std::cerr << "Error: No";
        switch (args.size()) {
        case 0:
            std::cerr << " file,";
            Q_FALLTHROUGH();
        case 1:
            std::cerr << " key and";
            Q_FALLTHROUGH();
        case 2:
            std::cerr << " values";
            break;
        }
        std::cerr << " given.\n" << std::endl;
        return false;
    }

    auto tempArgs = args;
    m_file = tempArgs.takeFirst();
    m_key = tempArgs.takeFirst();
    for (const auto &arg : tempArgs) {
        const auto val = Operation::valueFromString(arg);
        if (!val.isValid() || val.isNull()) {
            std::cerr << "Error: " << std::quoted(arg.toStdString())
                      << " is not a valid QVariant like string Type:Value.\n"
                      << std::endl;
            return false;
        }
        m_values.append(val);
    }

    return true;
}

int AddValueOperation::execute() const
{
    auto map = load(m_file);

    if (map.empty()) {
        std::cerr << "Error: Could not load " << m_file.toStdString() << std::endl;
        return FAILURE;
    }

    auto status = appendListToMap(map, m_key, m_values);

    if (status) {
        status = save(map, m_file);
    }

    return status ? SUCCESS : FAILURE;
}

#ifdef WITH_TESTS
bool AddValueOperation::test() const
{
    QVariantList testDataList;
    testDataList.append(QLatin1String("Some String"));
    testDataList.append(int(1860));

    KeyValuePairList testKvpList;
    testKvpList.append(KeyValuePair(QLatin1String("test/foo"), QString::fromLatin1("QString:Foo")));
    testKvpList.append(KeyValuePair(QLatin1String("test/foobar"), QString::fromLatin1("int:42")));
    testKvpList.append(KeyValuePair(QLatin1String("test/bar"), testDataList));

    const auto valueList = QVariantList(
        {Operation::valueFromString("QString:ELIL"), Operation::valueFromString("int:-1")});

    QVariantMap testMap;

    // add to empty map
    auto result = appendListToMap(testMap, "some key", valueList);

    if (result)
        return false;

    testMap.insert(QLatin1String("someEmptyThing"), QVariantMap());
    testMap.insert(QLatin1String("aKey"), "withAString");

    // append to a value
    result = appendListToMap(testMap, "aKey", valueList);

    if (result)
        return false;

    testMap = AddKeysOperation::addKeys(testMap, testKvpList);

    // quick sanity check
    if (testMap.count() != 3 && testDataList.count() != 2 && testKvpList.count() != 3)
        return false;

    // successful adding of values
    result = appendListToMap(testMap, "test/bar", valueList);
    if (!result)
        return false;

    const auto newList = qvariant_cast<QVariantList>(GetOperation::get(testMap, "test/bar"));
    if (newList.count() != (testDataList.count() + valueList.count()))
        return false;

    if (!newList.contains(1860) || !newList.contains(QString("Some String"))
        || !newList.contains("ELIL") || !newList.contains(-1))
        return false;

    return true;
}
#endif

bool AddValueOperation::appendListToMap(QVariantMap &map,
                                        const QString &key,
                                        const QVariantList &values)
{
    const auto data = GetOperation::get(map, key);

    if (!data.isValid() || data.isNull()) {
        std::cerr << "Error: Could not retrieve value for key " << std::quoted(key.toStdString())
                  << std::endl;
        return false;
    }

    if (!data.canConvert<QVariantList>()) {
        std::cerr << "Error: Data stored in " << std::quoted(key.toStdString())
                  << " can not be converted into QVariantList." << std::endl;
        return false;
    }

    auto newList = qvariant_cast<QVariantList>(data);

    newList.append(values);

    map = RmKeysOperation::rmKeys(map, {key});
    map = AddKeysOperation::addKeys(map, {Operation::KeyValuePair(key, newList)});

    return true;
}
