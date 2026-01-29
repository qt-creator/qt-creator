// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace QmlTraceViewer {

class QmlTraceViewerSettings : public Utils::AspectContainer
{
public:
    QmlTraceViewerSettings();

    Utils::FilePathAspect lastTraceFile{this};
    Utils::TypedAspect<QByteArray> windowGeometry{this};

    // Not persisted:
    Utils::BoolAspect exitOnError{this};
    Utils::BoolAspect printSourceLocations{this};
};

QmlTraceViewerSettings &settings();

} // namespace QmlTraceViewer
