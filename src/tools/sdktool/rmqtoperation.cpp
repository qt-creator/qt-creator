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

#include "rmqtoperation.h"

#include "addkeysoperation.h"
#include "addqtoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include <iostream>

#ifdef WITH_TESTS
#include <QTest>
#endif

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(rmqtlog, "qtc.sdktool.operations.rmqt", QtWarningMsg)

// ToolChain file stuff:
const char PREFIX[] = "QtVersion.";

// ToolChain:
const char AUTODETECTION_SOURCE[] = "autodetectionSource";

QString RmQtOperation::name() const
{
    return QLatin1String("rmQt");
}

QString RmQtOperation::helpText() const
{
    return QLatin1String("remove a Qt version");
}

QString RmQtOperation::argumentsHelpText() const
{
    return QLatin1String("    --id <ID>  The id of the qt version to remove.\n");
}

bool RmQtOperation::setArguments(const QStringList &args)
{
    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == QLatin1String("--id")) {
            if (next.isNull()) {
                qCCritical(rmqtlog) << "No parameter for --id given.";
                return false;
            }
            ++i; // skip next;
            m_id = next;
            continue;
        }
    }

    if (m_id.isEmpty())
        qCCritical(rmqtlog) << "No id given.";

    return !m_id.isEmpty();
}

int RmQtOperation::execute() const
{
    QVariantMap map = load(QLatin1String("QtVersions"));
    if (map.isEmpty())
        return 0;

    QVariantMap result = rmQt(map, m_id);
    if (result == map)
        return 2;

    return save(result, QLatin1String("QtVersions")) ? 0 : 3;
}

#ifdef WITH_TESTS
void RmQtOperation::unittest()
{
    // Add toolchain:
    QVariantMap map = AddQtData::initializeQtVersions();

    QVariantMap result = rmQt(QVariantMap(), QLatin1String("nonexistant"));
    QCOMPARE(result, map);

    AddQtData addData;
    addData.m_id = "testId";
    addData.m_displayName = "name";
    addData.m_type = "type";
    addData.m_qmake = "/tmp/test";
    addData.m_extra = {{QLatin1String("ExtraKey"), QVariant(QLatin1String("ExtraValue"))}};

    map = addData.addQt(map);

    addData.m_id = "testId2";
    addData.m_displayName = "other name";
    addData.m_type = "type";
    addData.m_qmake = "/tmp/test2";
    addData.m_extra = {};

    map = addData.addQt(map);

    result = rmQt(map, QLatin1String("nonexistant"));
    QCOMPARE(result, map);

    result = rmQt(map, QLatin1String("testId2"));
    QVERIFY(result != map);
    QVERIFY(result.contains(QLatin1String("QtVersion.0")));
    QCOMPARE(result.value(QLatin1String("QtVersion.0")), map.value(QLatin1String("QtVersion.0")));

    result = rmQt(map, QLatin1String("testId"));
    QVERIFY(result != map);
    QVERIFY(result.contains(QLatin1String("QtVersion.0")));
    QCOMPARE(result.value(QLatin1String("QtVersion.0")), map.value(QLatin1String("QtVersion.1")));

    result = rmQt(result, QLatin1String("testId2"));
    QVERIFY(result != map);
}
#endif

QVariantMap RmQtOperation::rmQt(const QVariantMap &map, const QString &id)
{
    QString sdkId = id;
    if (!id.startsWith(QLatin1String("SDK.")))
        sdkId = QString::fromLatin1("SDK.") + id;

    QVariantList qtList;
    for (QVariantMap::const_iterator i = map.begin(); i != map.end(); ++i) {
        if (!i.key().startsWith(QLatin1String(PREFIX)))
            continue;
        QVariantMap qtData = i.value().toMap();
        const QString dataId = qtData.value(QLatin1String(AUTODETECTION_SOURCE)).toString();
        if ((dataId != id) && (dataId != sdkId))
            qtList.append(qtData);
    }

    QVariantMap newMap = AddQtData::initializeQtVersions();
    for (int i = 0; i < qtList.count(); ++i)
        newMap.insert(QString::fromLatin1(PREFIX) + QString::number(i), qtList.at(i));

    return newMap;
}
