// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "secretaspect_test.h"

#include "secretaspect.h"

#include <utils/layoutbuilder.h>

#include <QLineEdit>
#include <QTest>

#include <memory>

using namespace Utils;

namespace Core::Internal {

// A value set with setValue() is handed out without asking the keychain, so
// none of these tests touches the secrets of the user.
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
};

QObject *createSecretAspectTest()
{
    return new SecretAspectTest;
}

} // namespace Core::Internal

#include "secretaspect_test.moc"
