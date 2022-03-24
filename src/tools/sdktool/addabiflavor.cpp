/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "addabiflavor.h"

#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#ifdef WITH_TESTS
#include <QtTest>
#endif

#include <iostream>

const char VERSION[] = "Version";
const char FLAVORS[] = "Flavors";
const char ABI_FILE_ID[] = "abi";

QString AddAbiFlavor::name() const
{
    return QLatin1String("addAbiFlavor");
}

QString AddAbiFlavor::helpText() const
{
    return QLatin1String("add an ABI flavor");
}

QString AddAbiFlavor::argumentsHelpText() const
{
    return QLatin1String("    --flavor <NAME>                            Name of new ABI flavor (required)\n"
                         "    --oses <OS>(,<OS>)*                        OSes the flavor applies to (required)\n");
}

bool AddAbiFlavor::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (next.isNull() && current.startsWith("--")) {
            std::cerr << "No parameter for option '" << qPrintable(current) << "' given." << std::endl << std::endl;
            return false;
        }

        if (current == QLatin1String("--flavor")) {
            ++i; // skip next;
            m_flavor = next;
            continue;
        }

        if (current == QLatin1String("--oses")) {
            ++i; // skip next;
            m_oses = next.split(',');
            continue;
        }

        std::cerr << "Unknown parameter: " << qPrintable(current) << std::endl << std::endl;
        return false;
    }

    if (m_flavor.isEmpty())
        std::cerr << "Error no flavor was passed." << std::endl << std::endl;

    if (m_oses.isEmpty())
        std::cerr << "Error no OSes name was passed." << std::endl << std::endl;

    return !m_flavor.isEmpty() && !m_oses.isEmpty();
}

int AddAbiFlavor::execute() const
{
    QVariantMap map = load(QLatin1String(ABI_FILE_ID));
    if (map.isEmpty())
        map = initializeAbiFlavors();

    QVariantMap result = addAbiFlavor(map);

    if (result.isEmpty() || result == map)
        return 2;

    return save(result, QLatin1String(ABI_FILE_ID)) ? 0 : 3;
}

#ifdef WITH_TESTS
bool AddAbiFlavor::test() const
{
    QVariantMap map = initializeAbiFlavors();
    if (map.count() != 1)
        return false;
    if (!map.contains(QLatin1String(VERSION)))
        return false;

    map = AddAbiFlavorData{{"linux", "windows"}, "foo"}.addAbiFlavor(map);

    if (map.count() != 2)
        return false;
    if (!map.contains(QLatin1String(VERSION)))
        return false;
    if (!map.contains(QLatin1String(FLAVORS)))
        return false;

    const QVariantMap flavorMap = map.value(QLatin1String(FLAVORS)).toMap();
    if (flavorMap.count() != 1)
        return false;
    if (flavorMap.value("foo").toStringList() != QStringList({"linux", "windows"}))
        return false;

    // Ignore known flavors:
    QTest::ignoreMessage(QtWarningMsg,
                         QRegularExpression("Error: flavor .* already defined as extra ABI flavor."));
    const QVariantMap result = AddAbiFlavorData({{"linux"}, "foo"}).addAbiFlavor(map);

    if (map != result)
        return false;

    return true;
}
#endif

QVariantMap AddAbiFlavorData::addAbiFlavor(const QVariantMap &map) const
{
    // Sanity check: Is flavor already set in abi file?
    if (exists(map, m_flavor)) {
        std::cerr << "Error: flavor " << qPrintable(m_flavor) << " already defined as extra ABI flavor." << std::endl;
        return map;
    }

    QVariantMap result = map;
    QVariantMap flavorMap = map.value(QLatin1String(FLAVORS)).toMap();
    flavorMap.insert(m_flavor, m_oses);
    result.insert(QLatin1String(FLAVORS), flavorMap);
    return result;
}

QVariantMap AddAbiFlavorData::initializeAbiFlavors()
{
    QVariantMap map;
    map.insert(QLatin1String(VERSION), 1);
    return map;
}

bool AddAbiFlavorData::exists(const QString &flavor)
{
    QVariantMap map = Operation::load(QLatin1String(ABI_FILE_ID));
    return exists(map, flavor);
}

bool AddAbiFlavorData::exists(const QVariantMap &map, const QString &flavor)
{
    const QVariantMap flavorMap = map.value(QLatin1String(FLAVORS)).toMap();
    return flavorMap.contains(flavor);
}
