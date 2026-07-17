// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "guitest.h"

#ifdef WITH_TESTS

#include "stringutils.h"

#include <QAbstractButton>
#include <QApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QPointer>
#include <QTest>
#include <QTimer>
#include <QWidget>

namespace Utils::GuiTest {

Matcher byType(const QByteArray &className)
{
    return Matcher([className](const QWidget *w) { return w->inherits(className.constData()); });
}

Matcher byObjectName(const QString &objectName)
{
    return Matcher([objectName](const QWidget *w) { return w->objectName() == objectName; });
}

Matcher byButtonText(const QString &text)
{
    return Matcher([text](const QWidget *w) {
        if (auto button = qobject_cast<const QAbstractButton *>(w))
            return Utils::stripAccelerator(button->text()) == text;
        return false;
    });
}

QWidget *findWidget(const Matcher &matcher, QWidget *root)
{
    if (root && root->isVisible() && matcher(root))
        return root;
    const QWidgetList widgets = root ? root->findChildren<QWidget *>() : QApplication::allWidgets();
    for (QWidget *w : widgets) {
        if (w->isVisible() && matcher(w))
            return w;
    }
    return nullptr;
}

bool waitFor(const std::function<bool()> &predicate, int timeoutMs)
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate()) {
        if (timer.elapsed() > timeoutMs)
            return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    return true;
}

QWidget *waitForWidget(const Matcher &matcher, QWidget *root, int timeoutMs)
{
    QWidget *found = nullptr;
    waitFor([&] { return (found = findWidget(matcher, root)) != nullptr; }, timeoutMs);
    return found;
}

// A widget that is null, hidden or disabled cannot be driven (QTest silently
// does nothing), so abort hard instead of letting the test pass without having
// interacted.
static void requireInteractable(const QWidget *widget, const char *func)
{
    if (!widget || !widget->isVisible() || !widget->isEnabled())
        qFatal("GuiTest::%s: widget is null, hidden or disabled", func);
}

void click(QWidget *widget, Qt::MouseButton button)
{
    requireInteractable(widget, "click");
    QTest::mouseClick(widget, button, {}, widget->rect().center());
}

void type(QWidget *widget, const QString &text)
{
    requireInteractable(widget, "type");
    QTest::keyClicks(widget, text);
}

void key(QWidget *widget, Qt::Key key, Qt::KeyboardModifiers modifiers)
{
    requireInteractable(widget, "key");
    QTest::keyClick(widget, key, modifiers);
}

DialogHandled onNextDialog(const std::function<void(QWidget *)> &interact, int timeoutMs)
{
    auto handled = std::make_shared<bool>(false);
    auto *timer = new QTimer(qApp);
    auto started = std::make_shared<QElapsedTimer>();
    started->start();
    QObject::connect(timer, &QTimer::timeout, timer, [=] {
        if (QWidget *dialog = QApplication::activeModalWidget()) {
            timer->stop();
            timer->deleteLater();
            QPointer<QWidget> guard(dialog);
            interact(dialog);
            if (guard && guard->isVisible())
                guard->close();
            *handled = true;
        } else if (started->elapsed() > timeoutMs) {
            timer->stop();
            timer->deleteLater();
            qWarning("GuiTest::onNextDialog: timed out waiting for a modal dialog");
        }
    });
    timer->start(10);
    return handled;
}

} // namespace Utils::GuiTest

#endif // WITH_TESTS
