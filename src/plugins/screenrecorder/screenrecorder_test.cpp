// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "screenrecorder_test.h"

#include <QTest>

namespace ScreenRecorder::Internal {

FFmpegOutputParserTest::FFmpegOutputParserTest(QObject *parent)
    : QObject(parent)
{ }

FFmpegOutputParserTest::~FFmpegOutputParserTest() = default;

} // namespace ScreenRecorder::Internal
