// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "mcutarget.h"

#include <QRegularExpression>

namespace McuSupport::Internal {

struct McuTargetDescription;

McuTarget::OS deduceOperatingSystem(const McuTargetDescription &);
QString removeRtosSuffix(const QString &environmentVariable);

} // namespace McuSupport::Internal
