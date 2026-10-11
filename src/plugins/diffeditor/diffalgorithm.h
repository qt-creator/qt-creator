// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/differ.h>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace DiffEditor::Internal {

Utils::Differ::DiffAlgorithm readDiffAlgorithm();
void saveDiffAlgorithm(Utils::Differ::DiffAlgorithm algorithm);
void populateDiffAlgorithms(QComboBox *comboBox);

} // namespace DiffEditor::Internal
