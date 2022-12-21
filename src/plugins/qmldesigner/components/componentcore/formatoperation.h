// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "selectioncontext.h"

namespace QmlDesigner {
namespace FormatOperation {

bool propertiesCopyable(const SelectionContext &selectionState);
bool propertiesApplyable(const SelectionContext &selectionState);
void copyFormat(const SelectionContext &selectionState);
void applyFormat(const SelectionContext &selectionState);
void readFormatConfiguration();

} // namespace FormatOperation
} // QmlDesigner
