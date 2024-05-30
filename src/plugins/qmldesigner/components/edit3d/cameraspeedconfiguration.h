// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include <auxiliarydata.h>

#include <QObject>
#include <QPoint>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QQuickView;
QT_END_NAMESPACE

namespace QmlDesigner {

class Edit3DView;

inline constexpr AuxiliaryDataKeyView edit3dCameraSpeedDocProperty{AuxiliaryDataType::Document,
                                                                   "cameraSpeed3d"};
inline constexpr AuxiliaryDataKeyView edit3dCameraSpeedMultiplierDocProperty{AuxiliaryDataType::Document,
                                                                             "cameraSpeed3dMultiplier"};
inline constexpr AuxiliaryDataKeyView edit3dCameraTotalSpeedProperty{AuxiliaryDataType::NodeInstanceAuxiliary,
                                                                     "cameraTotalSpeed3d"};

class CameraSpeedConfiguration : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(double multiplier READ multiplier WRITE setMultiplier NOTIFY multiplierChanged)
    Q_PROPERTY(double totalSpeed READ totalSpeed NOTIFY totalSpeedChanged)

public:
    CameraSpeedConfiguration(Edit3DView *view);
    ~CameraSpeedConfiguration();

    Q_INVOKABLE void resetDefaults();
    Q_INVOKABLE void hideCursor();
    Q_INVOKABLE void restoreCursor();
    Q_INVOKABLE void holdCursorInPlace();
    Q_INVOKABLE int devicePixelRatio();
    Q_INVOKABLE bool isQDSTrusted() const;

    void cancel();
    void apply();

    void showConfigDialog(const QPoint &pos);

    void setSpeed(double value);
    double speed() const { return m_speed; }
    void setMultiplier(double value);
    double multiplier() const { return m_multiplier; }
    double totalSpeed() const { return m_speed * m_multiplier; }

    constexpr static double defaultSpeed = 25.;
    constexpr static double defaultMultiplier = 1.;

signals:
    void speedChanged();
    void multiplierChanged();
    void totalSpeedChanged();
    void accessibilityOpened();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void asyncClose();

    QPointer<QQuickView> m_configDialog;
    QPointer<Edit3DView> m_view;
    double m_speed = 0.;
    double m_multiplier = 0.;
    bool m_changes = false;
    QPoint m_lastPos;
    bool m_cursorHidden = false;
};

} // namespace QmlDesigner
