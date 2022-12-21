// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>
#include <QString>

const char APPLICATION_NAME[] = "Qt Creator Crash Handler";
const char URL_BUGTRACKER[] = "https://bugreports.qt.io/";

QByteArray fileContents(const QString &filePath);
