// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMessageBox>

namespace Squish::Internal::SquishMessages {

void criticalMessage(const QString &title, const QString &details);
void criticalMessage(const QString &details);
void toolsInUnexpectedState(int state, const QString &additionalDetails);
QMessageBox::StandardButton simpleQuestion(const QString &title, const QString &detail);

} // Squish::Internal::SquishMessages
