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

#include "addkitoperation.h"

#include "addkeysoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include "settings.h"

#include <iostream>

// Qt version file stuff:
static char PREFIX[] = "Profile.";
static char VERSION[] = "Version";
static char COUNT[] = "Profile.Count";
static char DEFAULT[] = "Profile.Default";

// Kit:
static char ID[] = "PE.Profile.Id";
static char DISPLAYNAME[] = "PE.Profile.Name";
static char ICON[] = "PE.Profile.Icon";
static char AUTODETECTED[] = "PE.Profile.Autodetected";
static char DATA[] = "PE.Profile.Data";

// Standard KitInformation:
static char DEBUGGER[] = "Debugger.Information";
static char DEBUGGER_ENGINE[] = "EngineType";
static char DEBUGGER_BINARY[] = "Binary";
static char DEVICE_TYPE[] = "PE.Profile.DeviceType";
static char SYSROOT[] = "PE.Profile.SysRoot";
static char TOOLCHAIN[] = "PE.Profile.ToolChain";
static char MKSPEC[] = "QtPM4.mkSpecInformation";
static char QT[] = "QtSupport.QtInformation";

AddKitOperation::AddKitOperation()
    : m_debuggerEngine(0)
    , m_debugger(QLatin1String("auto"))
{
}

QString AddKitOperation::name() const
{
    return QLatin1String("addKit");
}

QString AddKitOperation::helpText() const
{
    return QLatin1String("add a Kit to Qt Creator");
}

QString AddKitOperation::argumentsHelpText() const
{
    return QLatin1String("    --id <ID>                                  id of the new kit.\n"
                         "    --name <NAME>                              display name of the new kit.\n"
                         "    --icon <PATH>                              icon of the new kit.\n"
                         "    --debuggerengine <ENGINE>                  debuggerengine of the new kit.\n"
                         "    --debugger <PATH>                          debugger of the new kit.\n"
                         "    --devicetype <TYPE>                        device type of the new kit.\n"
                         "    --sysroot <PATH>                           sysroot of the new kit.\n"
                         "    --toolchain <ID>                           tool chain of the new kit.\n"
                         "    --qt <ID>                                  Qt of the new kit.\n"
                         "    --mkspec <PATH>                            mkspec of the new kit.\n"
                         "    <KEY> <TYPE:VALUE>                         extra key value pairs\n");
}

bool AddKitOperation::setArguments(const QStringList &args)
{
    m_debuggerEngine = 0;

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

        if (current == QLatin1String("--name")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_displayName = next;
            continue;
        }

        if (current == QLatin1String("--icon")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_icon = next;
            continue;
        }

        if (current == QLatin1String("--debuggerengine")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            bool ok;
            m_debuggerEngine = next.toInt(&ok);
            if (!ok) {
                std::cerr << "Debugger type is not an integer!" << std::endl;
                return false;
            }
            continue;
        }

        if (current == QLatin1String("--debugger")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_debugger = next;
            continue;
        }

        if (current == QLatin1String("--devicetype")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_deviceType = next;
            continue;
        }

        if (current == QLatin1String("--sysroot")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sysRoot = next;
            continue;
        }

        if (current == QLatin1String("--toolchain")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_tc = next;
            continue;
        }

        if (current == QLatin1String("--qt")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_qt = next;
            continue;
        }

        if (current == QLatin1String("--mkspec")) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_mkspec = next;
            continue;
        }

        if (next.isNull())
            return false;
        ++i; // skip next;
        KeyValuePair pair(current, next);
        if (!pair.value.isValid())
            return false;
        m_extra << pair;
    }

    if (m_icon.isEmpty())
        m_icon = QLatin1String(":///DESKTOP///");

    return !m_id.isEmpty() && !m_displayName.isEmpty() && !m_deviceType.isEmpty();
}

int AddKitOperation::execute() const
{
    QVariantMap map = load(QLatin1String("profiles"));
    if (map.isEmpty())
        map = initializeKits();

    map = addKit(map, m_id, m_displayName, m_icon, m_debuggerEngine, m_debugger,
                 m_deviceType.toUtf8(), m_sysRoot, m_tc, m_qt, m_mkspec, m_extra);

    if (map.isEmpty())
        return -2;

    return save(map, QLatin1String("profiles")) ? 0 : -3;
}

#ifdef WITH_TESTS
bool AddKitOperation::test() const
{
    QVariantMap map = initializeKits();

    if (map.count() != 3
            || !map.contains(QLatin1String(VERSION))
            || map.value(QLatin1String(VERSION)).toInt() != 1
            || !map.contains(QLatin1String(COUNT))
            || map.value(QLatin1String(COUNT)).toInt() != 0
            || !map.contains(QLatin1String(DEFAULT))
            || map.value(QLatin1String(DEFAULT)).toInt() != -1)
        return false;

    map = addKit(map, QLatin1String("testId"), QLatin1String("Test Kit"), QLatin1String("/tmp/icon.png"),
                 1, QLatin1String("/usr/bin/gdb-test"),
                 QByteArray("Desktop"), QString(),
                 QLatin1String("{some-tc-id}"), QLatin1String("{some-qt-id}"), QLatin1String("unsupported/mkspec"),
                 KeyValuePairList() << KeyValuePair(QLatin1String("PE.Profile.Data/extraData"), QVariant(QLatin1String("extraValue"))));

    if (map.count() != 4
            || !map.contains(QLatin1String(VERSION))
            || map.value(QLatin1String(VERSION)).toInt() != 1
            || !map.contains(QLatin1String(COUNT))
            || map.value(QLatin1String(COUNT)).toInt() != 1
            || !map.contains(QLatin1String(DEFAULT))
            || map.value(QLatin1String(DEFAULT)).toInt() != 0
            || !map.contains(QLatin1String("Profile.0")))
        return false;

    QVariantMap profile0 = map.value(QLatin1String("Profile.0")).toMap();
    if (profile0.count() != 5
            || !profile0.contains(QLatin1String(ID))
            || profile0.value(QLatin1String(ID)).toString() != QLatin1String("testId")
            || !profile0.contains(QLatin1String(DISPLAYNAME))
            || profile0.value(QLatin1String(DISPLAYNAME)).toString() != QLatin1String("Test Kit")
            || !profile0.contains(QLatin1String(AUTODETECTED))
            || profile0.value(QLatin1String(AUTODETECTED)).toBool() != true)
        return false;

    // Ignore existing ids:
    QVariantMap result = addKit(map, QLatin1String("testId"), QLatin1String("Test Qt Version X"), QLatin1String("/tmp/icon3.png"),
                                1, QLatin1String("/usr/bin/gdb-test3"),
                                QByteArray("Desktop"), QString(),
                                QLatin1String("{some-tc-id3}"), QLatin1String("{some-qt-id3}"), QLatin1String("unsupported/mkspec3"),
                                KeyValuePairList() << KeyValuePair(QLatin1String("PE.Profile.Data/extraData"), QVariant(QLatin1String("extraValue3"))));
    if (!result.isEmpty())
        return false;

    // Make sure name is unique:
    map = addKit(map, QLatin1String("testId2"), QLatin1String("Test Kit2"), QLatin1String("/tmp/icon2.png"),
                 1, QLatin1String("/usr/bin/gdb-test2"),
                 QByteArray("Desktop"), QString(),
                 QLatin1String("{some-tc-id2}"), QLatin1String("{some-qt-id2}"), QLatin1String("unsupported/mkspec2"),
                 KeyValuePairList() << KeyValuePair(QLatin1String("PE.Profile.Data/extraData"), QVariant(QLatin1String("extraValue2"))));
    if (map.count() != 5
            || !map.contains(QLatin1String(VERSION))
            || map.value(QLatin1String(VERSION)).toInt() != 1
            || !map.contains(QLatin1String(COUNT))
            || map.value(QLatin1String(COUNT)).toInt() != 2
            || !map.contains(QLatin1String(DEFAULT))
            || map.value(QLatin1String(DEFAULT)).toInt() != 0
            || !map.contains(QLatin1String("Profile.0"))
            || !map.contains(QLatin1String("Profile.1")))

    if (map.value(QLatin1String("Profile.0")) != profile0)
        return false;

    QVariantMap profile1 = map.value(QLatin1String("Profile.1")).toMap();
    if (profile1.count() != 5
            || !profile1.contains(QLatin1String(ID))
            || profile1.value(QLatin1String(ID)).toString() != QLatin1String("testId2")
            || !profile1.contains(QLatin1String(DISPLAYNAME))
            || profile1.value(QLatin1String(DISPLAYNAME)).toString() != QLatin1String("Test Kit2")
            || !profile1.contains(QLatin1String(AUTODETECTED))
            || profile1.value(QLatin1String(AUTODETECTED)).toBool() != true)
        return false;

    return true;
}
#endif

QVariantMap AddKitOperation::addKit(const QVariantMap &map,
                                    const QString &id, const QString &displayName, const QString &icon,
                                    const quint32 &debuggerType, const QString &debugger,
                                    const QByteArray &deviceType, const QString &sysRoot,
                                    const QString &tc, const QString &qt, const QString &mkspec,
                                    const KeyValuePairList &extra)
{
    // Sanity check: Make sure autodetection source is not in use already:
    QStringList valueKeys = FindValueOperation::findValues(map, QVariant(id));
    bool hasId = false;
    foreach (const QString &k, valueKeys) {
        if (k.endsWith(QString(QLatin1Char('/')) + QLatin1String(ID))) {
            hasId = true;
            break;
        }
    }
    if (hasId) {
        std::cerr << "Error: Id " << qPrintable(id) << " already defined as kit." << std::endl;
        return QVariantMap();
    }

    // Find position to insert:
    bool ok;
    int count = GetOperation::get(map, QLatin1String(COUNT)).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Count found in kits file seems wrong." << std::endl;
        return QVariantMap();
    }
    const QString kit = QString::fromLatin1(PREFIX) + QString::number(count);

    int defaultKit = GetOperation::get(map, QLatin1String(DEFAULT)).toInt(&ok);
    if (!ok) {
        std::cerr << "Error: Default kit seems wrong." << std::endl;
        return QVariantMap();
    }

    if (defaultKit < 0)
        defaultKit = 0;

    // remove data:
    QStringList toRemove;
    toRemove << QLatin1String(COUNT) << QLatin1String(DEFAULT);
    QVariantMap cleaned = RmKeysOperation::rmKeys(map, toRemove);

    // Sanity check: Make sure displayName is unique.
    QStringList nameKeys = FindKeyOperation::findKey(map, QLatin1String(DISPLAYNAME));
    QStringList nameList;
    foreach (const QString nameKey, nameKeys)
        nameList << GetOperation::get(map, nameKey).toString();
    const QString uniqueName = makeUnique(displayName, nameList);

    QString qtId = qt;
    if (!qtId.startsWith(QLatin1String("SDK.")))
        qtId = QString::fromLatin1("SDK.") + qt;

    // insert data:
    KeyValuePairList data;
    data << KeyValuePair(QStringList() << kit << QLatin1String(ID), QVariant(id));
    data << KeyValuePair(QStringList() << kit << QLatin1String(DISPLAYNAME), QVariant(uniqueName));
    data << KeyValuePair(QStringList() << kit << QLatin1String(ICON), QVariant(icon));
    data << KeyValuePair(QStringList() << kit << QLatin1String(AUTODETECTED), QVariant(true));

    data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                         << QLatin1String(DEBUGGER) << QLatin1String(DEBUGGER_ENGINE), QVariant(debuggerType));
    data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                         << QLatin1String(DEBUGGER) << QLatin1String(DEBUGGER_BINARY), QVariant(debugger));
    data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                         << QLatin1String(DEVICE_TYPE), QVariant(deviceType));
    data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                         << QLatin1String(SYSROOT), QVariant(sysRoot));
    data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                         << QLatin1String(TOOLCHAIN), QVariant(tc));
    data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                         << QLatin1String(QT), QVariant(qtId));
    data << KeyValuePair(QStringList() << kit << QLatin1String(DATA)
                         << QLatin1String(MKSPEC), QVariant(mkspec));

    data << KeyValuePair(QStringList() << QLatin1String(DEFAULT), QVariant(defaultKit));
    data << KeyValuePair(QStringList() << QLatin1String(COUNT), QVariant(count + 1));

    KeyValuePairList qtExtraList;
    foreach (const KeyValuePair &pair, extra)
        qtExtraList << KeyValuePair(QStringList() << kit << pair.key, pair.value);
    data.append(qtExtraList);

    return AddKeysOperation::addKeys(cleaned, data);
}

QVariantMap AddKitOperation::initializeKits()
{
    QVariantMap map;
    map.insert(QLatin1String(VERSION), 1);
    map.insert(QLatin1String(DEFAULT), -1);
    map.insert(QLatin1String(COUNT), 0);
    return map;
}
