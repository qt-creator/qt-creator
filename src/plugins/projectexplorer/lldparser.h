// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ioutputparser.h"

namespace ProjectExplorer {
namespace Internal {

class LldParser : public OutputTaskParser
{
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
};

} // namespace Internal
} // namespace ProjectExplorer
