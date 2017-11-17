/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <utils/settingsaccessor.h>

#include <QtTest>

using namespace Utils;

// --------------------------------------------------------------------
// BasicTestSettingsAccessor:
// --------------------------------------------------------------------

class BasicTestSettingsAccessor : public Utils::SettingsAccessor
{
public:
    BasicTestSettingsAccessor(const QByteArray &id = QByteArray("test"));

    using Utils::SettingsAccessor::isBetterMatch;
    using Utils::SettingsAccessor::addVersionUpgrader;
};

BasicTestSettingsAccessor::BasicTestSettingsAccessor(const QByteArray &id) :
    Utils::SettingsAccessor(Utils::FileName::fromString("/foo/bar"))
{
    setDisplayName("Basic Test Settings Accessor");
    setApplicationDisplayName("SettingsAccessor Test (Basic)");
    setSettingsId(id);
}

// --------------------------------------------------------------------
// tst_SettingsAccessor:
// --------------------------------------------------------------------

class tst_SettingsAccessor : public QObject
{
    Q_OBJECT

private slots:
    void isBetterMatch();
    void isBetterMatch_idMismatch();
    void isBetterMatch_noId();
    void isBetterMatch_sameVersion();
    void isBetterMatch_emptyMap();
    void isBetterMatch_twoEmptyMaps();
};

static QVariantMap versionedMap(int version, const QByteArray &id = QByteArray(),
                                const QVariantMap &extra = QVariantMap())
{
    QVariantMap result;
    result.insert("Version", version);
    if (!id.isEmpty())
        result.insert("EnvironmentId", id);
    for (auto it = extra.cbegin(); it != extra.cend(); ++it)
        result.insert(it.key(), it.value());
    return result;
}

void tst_SettingsAccessor::isBetterMatch()
{
    const BasicTestSettingsAccessor accessor;

    const QVariantMap a = versionedMap(1, "test");
    const QVariantMap b = versionedMap(2, "test");

    QVERIFY(accessor.isBetterMatch(a, b));
    QVERIFY(!accessor.isBetterMatch(b, a));
}

void tst_SettingsAccessor::isBetterMatch_idMismatch()
{
    const BasicTestSettingsAccessor accessor;

    const QVariantMap a = versionedMap(1, "test");
    const QVariantMap b = versionedMap(2, "foo");

    QVERIFY(!accessor.isBetterMatch(a, b));
    QVERIFY(accessor.isBetterMatch(b, a));
}

void tst_SettingsAccessor::isBetterMatch_noId()
{
    const BasicTestSettingsAccessor accessor((QByteArray()));

    const QVariantMap a = versionedMap(1, "test");
    const QVariantMap b = versionedMap(2, "foo");

    QVERIFY(accessor.isBetterMatch(a, b));
    QVERIFY(!accessor.isBetterMatch(b, a));
}

void tst_SettingsAccessor::isBetterMatch_sameVersion()
{
    const BasicTestSettingsAccessor accessor;

    const QVariantMap a = versionedMap(10, "test");
    const QVariantMap b = versionedMap(10, "test");

    QVERIFY(!accessor.isBetterMatch(a, b));
    QVERIFY(!accessor.isBetterMatch(b, a));
}

void tst_SettingsAccessor::isBetterMatch_emptyMap()
{
    const BasicTestSettingsAccessor accessor;

    const QVariantMap a;
    const QVariantMap b = versionedMap(10, "test");

    QVERIFY(accessor.isBetterMatch(a, b));
    QVERIFY(!accessor.isBetterMatch(b, a));
}

void tst_SettingsAccessor::isBetterMatch_twoEmptyMaps()
{
    const BasicTestSettingsAccessor accessor;

    const QVariantMap a;
    const QVariantMap b;

    QVERIFY(accessor.isBetterMatch(a, b));
    QVERIFY(accessor.isBetterMatch(b, a));
}

QTEST_MAIN(tst_SettingsAccessor)

#include "tst_settings.moc"
