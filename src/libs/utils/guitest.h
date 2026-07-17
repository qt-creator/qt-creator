// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#ifdef WITH_TESTS

// In-process GUI-driving helpers for plugin tests: locate a widget by a
// Matcher, drive it via QTest, and interact with modal dialogs.

#include "utils_global.h"

#include <QtCore/qnamespace.h>

#include <functional>
#include <memory>

QT_BEGIN_NAMESPACE
class QByteArray;
class QString;
class QWidget;
QT_END_NAMESPACE

namespace Utils::GuiTest {

class Matcher
{
public:
    Matcher(std::function<bool(const QWidget *)> predicate)
        : m_predicate(std::move(predicate))
    {}

    bool operator()(const QWidget *widget) const { return m_predicate(widget); }

    Matcher operator&&(const Matcher &other) const
    {
        std::function<bool(const QWidget *)> a = m_predicate;
        std::function<bool(const QWidget *)> b = other.m_predicate;
        return Matcher([a, b](const QWidget *w) { return a(w) && b(w); });
    }

private:
    std::function<bool(const QWidget *)> m_predicate;
};

QTCREATOR_UTILS_EXPORT Matcher byType(const QByteArray &className);
QTCREATOR_UTILS_EXPORT Matcher byObjectName(const QString &objectName);
QTCREATOR_UTILS_EXPORT Matcher byButtonText(const QString &text);

QTCREATOR_UTILS_EXPORT QWidget *findWidget(const Matcher &matcher, QWidget *root = nullptr);
QTCREATOR_UTILS_EXPORT bool waitFor(const std::function<bool()> &predicate, int timeoutMs = 5000);
QTCREATOR_UTILS_EXPORT QWidget *waitForWidget(const Matcher &matcher, QWidget *root = nullptr,
                                              int timeoutMs = 5000);

QTCREATOR_UTILS_EXPORT void click(QWidget *widget, Qt::MouseButton button = Qt::LeftButton);
QTCREATOR_UTILS_EXPORT void type(QWidget *widget, const QString &text);
QTCREATOR_UTILS_EXPORT void key(QWidget *widget, Qt::Key key, Qt::KeyboardModifiers modifiers = {});

// Result handle for onNextDialog(): set once a modal dialog has been seen and
// driven. Check it (e.g. QVERIFY(*handle)) after the code that pops up the
// modal, so a dialog that never appeared does not let the test pass silently.
using DialogHandled = std::shared_ptr<bool>;

// Call before opening a modal: the poller fires inside the modal's own exec()
// loop. interact() is expected to close the dialog; if it does not, the dialog
// is closed here as a safety net so the exec() loop unblocks and the test fails
// instead of hanging.
QTCREATOR_UTILS_EXPORT DialogHandled onNextDialog(const std::function<void(QWidget *)> &interact,
                                                  int timeoutMs = 5000);

} // namespace Utils::GuiTest

#endif // WITH_TESTS
