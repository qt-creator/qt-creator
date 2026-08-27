// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/filepath.h>
#include <utils/pathchooser.h>

#include <QDir>
#include <QMetaMethod>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTest>
#include <QWidget>

#include <memory>
#include <vector>

using namespace Utils;

class tst_PathChooser : public QObject
{
    Q_OBJECT

private slots:
    void testSilenceWhileBeingDestroyed();
    void testForeignConnectionsAreLeftAlone();
};

// A chooser deleted while it holds the focus - what a settings panel does to its
// subwidgets when it rebuilds them - clears that focus from its destructor, which
// makes the line edit emit. Every signal of this class is relayed from the line
// edit, so a receiver would be handed a chooser that is already being destroyed.
void tst_PathChooser::testSilenceWhileBeingDestroyed()
{
    auto parent = new QWidget;
    auto chooser = new PathChooser(parent);
    // An unacceptable input keeps the line edit from emitting on focus-out at all,
    // so an empty path would let this test pass no matter what.
    chooser->setFilePath(FilePath::fromString(QDir::tempPath()));
    parent->show();
    QVERIFY(QTest::qWaitForWindowActive(parent));

    chooser->setFocus();
    QVERIFY(chooser->hasFocus()); // Without focus there is nothing to emit from.

    std::vector<std::unique_ptr<QSignalSpy>> spies;
    const QMetaObject *metaObject = &PathChooser::staticMetaObject;
    for (int i = metaObject->methodOffset(); i < metaObject->methodCount(); ++i) {
        const QMetaMethod method = metaObject->method(i);
        if (method.methodType() != QMetaMethod::Signal)
            continue;
        spies.push_back(std::make_unique<QSignalSpy>(chooser, method));
        QVERIFY2(spies.back()->isValid(), method.methodSignature());
    }
    QVERIFY(!spies.empty());

    delete chooser;

    for (const std::unique_ptr<QSignalSpy> &spy : spies)
        QCOMPARE(spy->count(), 0);

    delete parent;
}

static QStringList s_warnings;

// Silencing the relays must not take anyone else's connections to the line edit
// with it. A wildcard disconnect does, and Qt warns about dropping destroyed().
void tst_PathChooser::testForeignConnectionsAreLeftAlone()
{
    s_warnings.clear();
    const QtMessageHandler previous = qInstallMessageHandler(
        [](QtMsgType type, const QMessageLogContext &, const QString &message) {
            if (type == QtWarningMsg)
                s_warnings << message;
        });
    const QScopeGuard restore([previous] { qInstallMessageHandler(previous); });

    auto parent = new QWidget;
    auto chooser = new PathChooser(parent);
    chooser->setFilePath(FilePath::fromString(QDir::tempPath()));

    bool lineEditDestroyed = false;
    QObject::connect(chooser->lineEdit(), &QObject::destroyed, [&lineEditDestroyed] {
        lineEditDestroyed = true;
    });

    delete chooser;

    QVERIFY(lineEditDestroyed);
    QVERIFY2(s_warnings.isEmpty(), qPrintable(s_warnings.join(", ")));

    delete parent;
}

QTEST_MAIN(tst_PathChooser)

#include "tst_pathchooser.moc"
