// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/id.h>

#include <functional>

#include <QWidget>

class QCheckBox;
class QLabel;

namespace ProjectExplorer {

PROJECTEXPLORER_EXPORT QLabel *createGlobalSettingsLink(Utils::Id globalId);

PROJECTEXPLORER_EXPORT QWidget *createGlobalOrProjectSelector(
    QObject *guard,
    bool initialUseGlobal,
    Utils::Id globalId,
    std::function<void(bool)> onChange,
    QCheckBox **checkBoxOut = nullptr);

} // namespace ProjectExplorer
