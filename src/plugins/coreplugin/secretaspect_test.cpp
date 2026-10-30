// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "secretaspect_test.h"

#include "secretaspect.h"

#include <utils/layoutbuilder.h>
#include <utils/qtcsettings.h>

#include <QLineEdit>
#include <QScopeGuard>
#include <QTest>

#include <memory>

using namespace Utils;

namespace Core::Internal {

static QString valueOf(const SecretAspect &aspect)
{
    QString result;
    aspect.requestValue([&result](const Result<QString> &value) {
        if (value)
            result = *value;
    });
    return result;
}

static std::unique_ptr<QWidget> createWidget(const SecretAspect &aspect)
{
    Layouting::Form form;
    aspect.addToLayout(form);
    return std::unique_ptr<QWidget>(form.emerge());
}

class SecretAspectTest final : public QObject
{
    Q_OBJECT

private slots:
    // Keeps the tests away from the keychain of the host, which they would
    // otherwise probe and possibly read.
    void initTestCase() { setKeychainAvailableForTest(false); }
    void cleanupTestCase() { setKeychainAvailableForTest(std::nullopt); }

    void testCancelDiscardsEdit()
    {
        SecretAspect aspect;
        aspect.setAutoApply(false);
        aspect.setValue("applied");
        const std::unique_ptr<QWidget> widget = createWidget(aspect);
        auto edit = widget->findChild<QLineEdit *>();
        QVERIFY(edit);
        QCOMPARE(edit->text(), "applied");

        QTest::keyClicks(edit, "-typed");
        QCOMPARE(valueOf(aspect), "applied");

        aspect.cancel();
        QCOMPARE(edit->text(), "applied");
        QCOMPARE(valueOf(aspect), "applied");
        aspect.apply();
        QCOMPARE(valueOf(aspect), "applied");
    }

    void testApplyTakesEdit()
    {
        SecretAspect aspect;
        aspect.setAutoApply(false);
        aspect.setValue("applied");
        const std::unique_ptr<QWidget> widget = createWidget(aspect);
        auto edit = widget->findChild<QLineEdit *>();
        QVERIFY(edit);

        QTest::keyClicks(edit, "-typed");
        QCOMPARE(valueOf(aspect), "applied");
        aspect.apply();
        QCOMPARE(valueOf(aspect), "applied-typed");
    }

    void testAutoApplyTakesEditRightAway()
    {
        SecretAspect aspect;
        aspect.setValue("applied");
        const std::unique_ptr<QWidget> widget = createWidget(aspect);
        auto edit = widget->findChild<QLineEdit *>();
        QVERIFY(edit);

        QTest::keyClicks(edit, "-typed");
        QCOMPARE(valueOf(aspect), "applied-typed");
    }

    void testPlaintextSecretIsReadBack()
    {
        const char group[] = "CoreTestsSecretAspect";
        const auto cleanup = qScopeGuard([group] { Utils::userSettings().remove(group); });

        {
            AspectContainer container;
            container.setSettingsGroup(group);
            SecretAspect aspect(&container);
            aspect.setSettingsKey("Password");
            container.readSettings();
            aspect.setValue("stored");
            container.writeSettings();
            QCOMPARE(valueOf(aspect), "stored");
        }

        AspectContainer container;
        container.setSettingsGroup(group);
        SecretAspect aspect(&container);
        aspect.setSettingsKey("Password");
        container.readSettings();
        QCOMPARE(valueOf(aspect), "stored");
    }
};

QObject *createSecretAspectTest()
{
    return new SecretAspectTest;
}

} // namespace Core::Internal

#include "secretaspect_test.moc"
