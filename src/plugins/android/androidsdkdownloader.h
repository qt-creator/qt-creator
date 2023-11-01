// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "androidconfigurations.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QProgressDialog;
QT_END_NAMESPACE

namespace Tasking { class TaskTree; }

namespace Android::Internal {

class AndroidSdkDownloader : public QObject
{
    Q_OBJECT

public:
    AndroidSdkDownloader();
    ~AndroidSdkDownloader();

    void downloadAndExtractSdk();
    static QString dialogTitle();

signals:
    void sdkExtracted();
    void sdkDownloaderError(const QString &error);

private:
    void logError(const QString &error);

    AndroidConfig &m_androidConfig;
    std::unique_ptr<QProgressDialog> m_progressDialog;
    std::unique_ptr<Tasking::TaskTree> m_taskTree;
};

} // namespace Android::Internal
