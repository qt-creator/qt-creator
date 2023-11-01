// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace ScreenRecorder::Internal {

class FFmpegOutputParserTest : public QObject
{
    Q_OBJECT

public:
    FFmpegOutputParserTest(QObject *parent = nullptr);
    ~FFmpegOutputParserTest();

private slots:
    void testVersionParser_data();
    void testVersionParser();
    void testClipInfoParser_data();
    void testClipInfoParser();
    void testFfmpegOutputParser_data();
    void testFfmpegOutputParser();
};

} // namespace ScreenRecorder::Internal
