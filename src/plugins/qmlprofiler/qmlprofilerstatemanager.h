// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace QmlProfiler {

class QmlProfilerStateManager : public QObject
{
    Q_OBJECT
public:
    enum QmlProfilerState {
        Idle,
        AppRunning,
        AppStopRequested,
        AppDying,
    };

    explicit QmlProfilerStateManager(QObject *parent = nullptr);
    ~QmlProfilerStateManager() override;

    QmlProfilerState currentState();
    bool clientRecording();
    bool serverRecording();
    quint64 requestedFeatures() const;
    quint64 recordedFeatures() const;

    QString currentStateAsString();

    void setCurrentState(QmlProfilerState newState);
    void setClientRecording(bool recording);
    void setServerRecording(bool recording);
    void setRequestedFeatures(quint64 features);
    void setRecordedFeatures(quint64 features);

signals:
    void stateChanged();
    void clientRecordingChanged(bool);
    void serverRecordingChanged(bool);
    void requestedFeaturesChanged(quint64);
    void recordedFeaturesChanged(quint64);

private:
    class QmlProfilerStateManagerPrivate;
    QmlProfilerStateManagerPrivate *d;
};

} // namespace QmlProfiler
