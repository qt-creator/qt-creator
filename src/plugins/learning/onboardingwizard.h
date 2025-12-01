// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

namespace Utils::StyleHelper {
class TextFormat;
}

namespace Learning::Internal {

QWidget *createOnboardingWizard(QWidget *parent);

QWidget *tfLabel(const QString &text, const Utils::StyleHelper::TextFormat &format);

} // namespace Learning::Internal
