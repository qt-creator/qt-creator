// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QObject;
QT_END_NAMESPACE

namespace WebAssembly::Internal {

using WebBrowserEntry = QPair<QString, QString>; // first: id, second: display name
using WebBrowserEntries = QList<WebBrowserEntry>;

WebBrowserEntries parseEmrunOutput(const QByteArray &output);

void setupEmrunRunSupport();

QObject *createWebBrowserSelectionAspectTest();

} // Webassembly::Internal
