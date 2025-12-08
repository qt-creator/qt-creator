// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringList>

namespace QmlDesigner::AiAssistantUtils {

QString currentQmlText();
QStringList currentSelectedIds();

QString reformatQml(const QString &content);

void setQml(const QString &qml);
void selectIds(const QStringList &ids);

bool isValidQmlCode(const QString &qmlCode);

} // namespace QmlDesigner::AiAssistantUtils
