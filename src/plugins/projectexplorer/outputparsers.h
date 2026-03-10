// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

QT_FORWARD_DECLARE_CLASS(QObject)

namespace Utils { class OutputLineParser; }

namespace ProjectExplorer {

PROJECTEXPLORER_EXPORT Utils::OutputLineParser *createOsOutputParser();
PROJECTEXPLORER_EXPORT Utils::OutputLineParser *createGenericOutputParser();

#ifdef WITH_TESTS
namespace Internal { QObject *createGenericOutputParserTest(); }
#endif

} // namespace ProjectExplorer
