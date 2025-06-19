// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

namespace Utils {
class TextFormat;
}

namespace Learning::Internal {

QWidget *createOnboardingWizard(QWidget *parent);

QWidget *tfLabel(const QString &text, const Utils::TextFormat &format);

} // namespace Learning::Internal
