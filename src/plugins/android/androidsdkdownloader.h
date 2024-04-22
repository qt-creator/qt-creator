// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <solutions/tasking/tasktreerunner.h>

QT_BEGIN_NAMESPACE
class QProgressDialog;
QT_END_NAMESPACE

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

private:
    std::unique_ptr<QProgressDialog> m_progressDialog;
    Tasking::TaskTreeRunner m_taskTreeRunner;
};

} // namespace Android::Internal
