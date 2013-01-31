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

#include "rmqtoperation.h"

#include "addkeysoperation.h"
#include "addqtoperation.h"
#include "findkeyoperation.h"
#include "findvalueoperation.h"
#include "getoperation.h"
#include "rmkeysoperation.h"

#include <iostream>

// ToolChain file stuff:
static char PREFIX[] = "QtVersion.";

// ToolChain:
static char AUTODETECTION_SOURCE[] = "autodetectionSource";

QString RmQtOperation::name() const
{
    return QLatin1String("rmQt");
}

QString RmQtOperation::helpText() const
{
    return QLatin1String("remove a Qt version from Qt Creator");
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
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_id = next;
            continue;
        }
    }

    return !m_id.isEmpty();
}

int RmQtOperation::execute() const
{
    QVariantMap map = load(QLatin1String("qtversion"));
    if (map.isEmpty())
        return 0;

    QVariantMap result = rmQt(map, m_id);
    if (result == map)
        return -2;

    return save(result, QLatin1String("qtversion")) ? 0 : -3;
}

#ifdef WITH_TESTS
bool RmQtOperation::test() const
{
    // Add toolchain:
    QVariantMap map = AddQtOperation::initializeQtVersions();

    QVariantMap result = rmQt(QVariantMap(), QLatin1String("nonexistant"));
    if (result != map)
        return false;

    map = AddQtOperation::addQt(map, QLatin1String("testId"), QLatin1String("name"), QLatin1String("type"),
                                QLatin1String("/tmp/test"),
                                KeyValuePairList() << KeyValuePair(QLatin1String("ExtraKey"), QVariant(QLatin1String("ExtraValue"))));
    map = AddQtOperation::addQt(map, QLatin1String("testId2"), QLatin1String("other name"),  QLatin1String("type"),
                                QLatin1String("/tmp/test2"),
                                KeyValuePairList());

    result = rmQt(map, QLatin1String("nonexistant"));
    if (result != map)
        return false;

    result = rmQt(map, QLatin1String("testId2"));
    if (result == map
            || !result.contains(QLatin1String("QtVersion.0"))
            || result.value(QLatin1String("QtVersion.0")) != map.value(QLatin1String("QtVersion.0")))
        return false;

    result = rmQt(map, QLatin1String("testId"));
    if (result == map
            || !result.contains(QLatin1String("QtVersion.0"))
            || result.value(QLatin1String("QtVersion.0")) != map.value(QLatin1String("QtVersion.1")))
        return false;

    result = rmQt(result, QLatin1String("testId2"));
    if (result == map)
        return false;

    return true;
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

    QVariantMap newMap = AddQtOperation::initializeQtVersions();
    for (int i = 0; i < qtList.count(); ++i)
        newMap.insert(QString::fromLatin1(PREFIX) + QString::number(i), qtList.at(i));

    return newMap;
}

