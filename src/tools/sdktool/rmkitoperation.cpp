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

#include "rmkitoperation.h"

#include "addkeysoperation.h"
#include "addkitoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <iostream>

// Qt version file stuff:
static char PREFIX[] = "Profile.";
static char COUNT[] = "Profile.Count";
static char DEFAULT[] = "Profile.Default";
#ifdef WITH_TESTS
static char VERSION[] = "Version";
#endif

// Kit:
static char ID[] = "PE.Profile.Id";

QString RmKitOperation::name() const
{
    return QLatin1String("rmKit");
}

QString RmKitOperation::helpText() const
{
    return QLatin1String("remove a Kit from Qt Creator");
}

QString RmKitOperation::argumentsHelpText() const
{
    return QLatin1String("    --id <ID>                                  id of the new kit.\n");
}

bool RmKitOperation::setArguments(const QStringList &args)
{
    if (args.count() != 2)
        return false;
    if (args.at(0) != QLatin1String("--id"))
        return false;

    m_id = args.at(1);

    return !m_id.isEmpty();
}

int RmKitOperation::execute() const
{
    QVariantMap map = load(QLatin1String("profiles"));
    if (map.isEmpty())
        map = AddKitOperation::initializeKits();

    map = rmKit(map, m_id);

    if (map.isEmpty())
        return -2;

    return save(map, QLatin1String("profiles")) ? 0 : -3;
}

#ifdef WITH_TESTS
bool RmKitOperation::test() const
{
    QVariantMap map =
            AddKitOperation::addKit(AddKitOperation::initializeKits(),
                                    QLatin1String("testId"), QLatin1String("Test Qt Version"),
                                    QLatin1String("/tmp/icon.png"),
                                    1, QLatin1String("/usr/bin/gdb-test"),
                                    QByteArray("Desktop"), QString(),
                                    QLatin1String("{some-tc-id}"), QLatin1String("{some-qt-id}"),
                                    QLatin1String("unsupported/mkspec"),
                                    KeyValuePairList() << KeyValuePair(QLatin1String("PE.Profile.Data/extraData"), QVariant(QLatin1String("extraValue"))));
    map =
            AddKitOperation::addKit(map, QLatin1String("testId2"), QLatin1String("Test Qt Version"),
                                    QLatin1String("/tmp/icon2.png"),
                                    1, QLatin1String("/usr/bin/gdb-test2"),
                                    QByteArray("Desktop"), QString(),
                                    QLatin1String("{some-tc-id2}"), QLatin1String("{some-qt-id2}"),
                                    QLatin1String("unsupported/mkspec2"),
                                    KeyValuePairList() << KeyValuePair(QLatin1String("PE.Profile.Data/extraData"), QVariant(QLatin1String("extraValue2"))));

    QVariantMap result = rmKit(map, QLatin1String("testId"));
    if (result.count() != 4
            || !result.contains(QLatin1String("Profile.0"))
            || !result.contains(QLatin1String(COUNT))
            || result.value(QLatin1String(COUNT)).toInt() != 1
            || !result.contains(QLatin1String(DEFAULT))
            || result.value(QLatin1String(DEFAULT)).toInt() != 0
            || !result.contains(QLatin1String(VERSION))
            || result.value(QLatin1String(VERSION)).toInt() != 1)
        return false;

    result = rmKit(map, QLatin1String("unknown"));
    if (result != map)
        return false;

    result = rmKit(map, QLatin1String("testId2"));
    if (result.count() != 4
            || !result.contains(QLatin1String("Profile.0"))
            || !result.contains(QLatin1String(COUNT))
            || result.value(QLatin1String(COUNT)).toInt() != 1
            || !result.contains(QLatin1String(DEFAULT))
            || result.value(QLatin1String(DEFAULT)).toInt() != 0
            || !result.contains(QLatin1String(VERSION))
            || result.value(QLatin1String(VERSION)).toInt() != 1)
        return false;

    result = rmKit(result, QLatin1String("testId"));
    if (result.count() != 3
            || !result.contains(QLatin1String(COUNT))
            || result.value(QLatin1String(COUNT)).toInt() != 0
            || !result.contains(QLatin1String(DEFAULT))
            || result.value(QLatin1String(DEFAULT)).toInt() != -1
            || !result.contains(QLatin1String(VERSION))
            || result.value(QLatin1String(VERSION)).toInt() != 1)
        return false;

    return true;
}
#endif

QVariantMap RmKitOperation::rmKit(const QVariantMap &map, const QString &id)
{
    QVariantMap result = AddKitOperation::initializeKits();

    QVariantList profileList;
    bool ok;
    int count = GetOperation::get(map, QLatin1String(COUNT)).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: The count found in map is not an integer.";
        return map;
    }

    int kitPos = -1;
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(PREFIX) + QString::number(i);
        QVariantMap profile = map.value(key).toMap();
        if (profile.value(QLatin1String(ID)).toString() == id) {
            kitPos = i;
            continue;
        }
        profileList << profile;
    }
    if (profileList.count() == map.count() - 3) {
        std::cerr << "Error: Id was not found.";
        return map;
    }

    int defaultKit = GetOperation::get(map, QLatin1String(DEFAULT)).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: Could not find the default kit.";
        defaultKit = -1;
    }

    if (defaultKit == kitPos || defaultKit < 0)
        defaultKit = (count > 1) ? 0 : -1;

    // remove data:
    QStringList toRemove;
    toRemove << QLatin1String(COUNT) << QLatin1String(DEFAULT);
    result = RmKeysOperation::rmKeys(result, toRemove);

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(QStringList() << QLatin1String(DEFAULT), QVariant(defaultKit));
    data << KeyValuePair(QStringList() << QLatin1String(COUNT), QVariant(count - 1));

    for (int i = 0; i < profileList.count(); ++i)
        data << KeyValuePair(QStringList() << QString::fromLatin1(PREFIX) + QString::number(i),
                             profileList.at(i));

    return AddKeysOperation::addKeys(result, data);
}

