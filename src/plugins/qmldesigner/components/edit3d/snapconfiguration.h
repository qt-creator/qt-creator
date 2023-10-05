// Copyright (C) 2023 The Qt Company Ltd.
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

inline constexpr AuxiliaryDataKeyView edit3dSnapPosProperty{AuxiliaryDataType::NodeInstanceAuxiliary,
                                                            "snapPos3d"};
inline constexpr AuxiliaryDataKeyView edit3dSnapPosIntProperty{AuxiliaryDataType::NodeInstanceAuxiliary,
                                                               "snapPosInt3d"};
inline constexpr AuxiliaryDataKeyView edit3dSnapAbsProperty{AuxiliaryDataType::NodeInstanceAuxiliary,
                                                            "snapAbs3d"};
inline constexpr AuxiliaryDataKeyView edit3dSnapRotProperty{AuxiliaryDataType::NodeInstanceAuxiliary,
                                                            "snapRot3d"};
inline constexpr AuxiliaryDataKeyView edit3dSnapRotIntProperty{AuxiliaryDataType::NodeInstanceAuxiliary,
                                                               "snapRotInt3d"};
inline constexpr AuxiliaryDataKeyView edit3dSnapScaleProperty{AuxiliaryDataType::NodeInstanceAuxiliary,
                                                              "snapScale3d"};
inline constexpr AuxiliaryDataKeyView edit3dSnapScaleIntProperty{AuxiliaryDataType::NodeInstanceAuxiliary,
                                                                 "snapScaleInt3d"};

class SnapConfiguration : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool posEnabled READ posEnabled WRITE setPosEnabled NOTIFY posEnabledChanged)
    Q_PROPERTY(bool rotEnabled READ rotEnabled WRITE setRotEnabled NOTIFY rotEnabledChanged)
    Q_PROPERTY(bool scaleEnabled READ scaleEnabled WRITE setScaleEnabled NOTIFY scaleEnabledChanged)
    Q_PROPERTY(bool absolute READ absolute WRITE setAbsolute NOTIFY absoluteChanged)
    Q_PROPERTY(double posInt READ posInt WRITE setPosInt NOTIFY posIntChanged)
    Q_PROPERTY(double rotInt READ rotInt WRITE setRotInt NOTIFY rotIntChanged)
    Q_PROPERTY(double scaleInt READ scaleInt WRITE setScaleInt NOTIFY scaleIntChanged)

public:
    SnapConfiguration(Edit3DView *view);
    ~SnapConfiguration();

    Q_INVOKABLE void resetDefaults();
    Q_INVOKABLE void hideCursor();
    Q_INVOKABLE void restoreCursor();
    Q_INVOKABLE void holdCursorInPlace();
    Q_INVOKABLE int devicePixelRatio();

    void cancel();
    void apply();

    void showConfigDialog(const QPoint &pos);

    void setPosEnabled(bool enabled);
    bool posEnabled() const { return m_positionEnabled; }
    void setRotEnabled(bool enabled);
    bool rotEnabled() const { return m_rotationEnabled; }
    void setScaleEnabled(bool enabled);
    bool scaleEnabled() const { return m_scaleEnabled; }
    void setAbsolute(bool enabled);
    bool absolute() const { return m_absolute; }

    void setPosInt(double value);
    double posInt() const { return m_positionInterval; }
    void setRotInt(double value);
    double rotInt() const { return m_rotationInterval; }
    void setScaleInt(double value);
    double scaleInt() const { return m_scaleInterval; }

    constexpr static double defaultPosInt = 50.;
    constexpr static double defaultRotInt = 5.;
    constexpr static double defaultScaleInt = 10.;

signals:
    void posEnabledChanged();
    void rotEnabledChanged();
    void scaleEnabledChanged();
    void absoluteChanged();
    void posIntChanged();
    void rotIntChanged();
    void scaleIntChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void asyncClose();

    QPointer<QQuickView> m_configDialog;
    QPointer<Edit3DView> m_view;
    bool m_positionEnabled = true;
    bool m_rotationEnabled = true;
    bool m_scaleEnabled = true;
    bool m_absolute = true;
    double m_positionInterval = 0.;
    double m_rotationInterval = 0.;
    double m_scaleInterval = 0.;
    bool m_changes = false;
    QPoint m_lastPos;
};

} // namespace QmlDesigner
