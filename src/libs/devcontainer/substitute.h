// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace DevContainer::Internal {

using Replacers = QMap<QString, std::function<QString(QStringList)>>;
void substituteVariables(QString &str, const Replacers &replacers);

} // namespace DevContainer::Internal
