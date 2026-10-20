// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QObject>
#include <QSize>
#include <QVariantMap>

#include <functional>

namespace Utils { class Process; }

namespace ScreenRecorder::Internal {

class PortalCapture : public QObject
{
    Q_OBJECT

public:
    explicit PortalCapture(QObject *parent = nullptr);
    ~PortalCapture() override;

    void prepare(const Utils::FilePath &fifoDirectory, int frameRate, bool captureCursor);
    void stop();

    QStringList ffmpegGrabArguments() const;

signals:
    void prepared();
    void cancelled();
    void failed(const QString &message);

private slots:
    void onResponse(uint code, const QVariantMap &results);

private:
    using ResultHandler = std::function<void(const QVariantMap &)>;

    void callPortal(const QString &method, QVariantList arguments, QVariantMap options,
                    const ResultHandler &onSuccess);
    void selectSources(bool captureCursor);
    void startStream();
    void startPipeline(const QString &pipeWireSource);

    QString m_sessionHandle;
    QString m_requestPath;
    ResultHandler m_onSuccess;
    Utils::FilePath m_fifo;
    Utils::FilePath m_gstLaunch;
    QSize m_streamSize;
    int m_frameRate = 0;
    Utils::Process *m_pipeline = nullptr;
};

} // namespace ScreenRecorder::Internal
