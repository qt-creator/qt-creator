// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QString;
class QWidget;
QT_END_NAMESPACE

namespace FakeVim::Internal {
class FakeVimHandler;
using SetupTestCallback = void (*)(QString *, FakeVimHandler **, QWidget **);
QObject *createFakeVimTester(SetupTestCallback cb);
} // namespace FakeVim::Internal
