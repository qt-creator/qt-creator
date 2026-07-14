// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <QStringList>

#include <functional>

QT_BEGIN_NAMESPACE
class QFont;
class QObject;
QT_END_NAMESPACE

namespace Core::MessageManager {

CORE_EXPORT void setFont(const QFont &font);
CORE_EXPORT void setWheelZoomEnabled(bool enabled);
CORE_EXPORT void popup();

// Registers an observer that is notified, on the GUI thread, of every message
// written to the General Messages pane. The connection lives as long as the
// guard object.
using OutputObserver = std::function<void(const QString &message)>;
CORE_EXPORT void addObserver(QObject *guard, const OutputObserver &observer);

CORE_EXPORT void clearLinesPrefixedWith(const QString &prefix, bool deleteTrailingLineBreak);

CORE_EXPORT void writeSilently(const QString &message);
CORE_EXPORT void writeFlashing(const QString &message);
CORE_EXPORT void writeDisrupting(const QString &message);

CORE_EXPORT void writeSilently(const QStringList &messages);
CORE_EXPORT void writeFlashing(const QStringList &messages);
CORE_EXPORT void writeDisrupting(const QStringList &messages);

} // namespace Core::MessageManager
