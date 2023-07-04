// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ffmpegutils.h"

#include <utils/styledbar.h>

namespace Utils {
class Process;
}

namespace ScreenRecorder {

class RecordOptionsDialog;

class RecordWidget : public Utils::StyledBar
{
    Q_OBJECT

public:
    explicit RecordWidget(const Utils::FilePath &recordFile, QWidget *parent = nullptr);
    ~RecordWidget();

    static QString recordFileExtension();

signals:
    void started();
    void finished(const ClipInfo &clip);

private:
    QStringList ffmpegParameters(const ClipInfo &clipInfo) const;

    const Utils::FilePath m_recordFile;
    ClipInfo m_clipInfo;
    Utils::Process *m_process;
    QByteArray m_lastOutputChunk;
    RecordOptionsDialog *m_optionsDialog;
    QAction *m_openClipAction;
};

} // namespace ScreenRecorder
