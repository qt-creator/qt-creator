// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include <auxiliarydata.h>

#include <QObject>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QQuickView;
class QPoint;
QT_END_NAMESPACE

namespace QmlDesigner {

class AbstractView;

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
    Q_PROPERTY(double posInt READ posInt WRITE setPosInt NOTIFY posIntChanged)
    Q_PROPERTY(double rotInt READ rotInt WRITE setRotInt NOTIFY rotIntChanged)
    Q_PROPERTY(double scaleInt READ scaleInt WRITE setScaleInt NOTIFY scaleIntChanged)

public:
    SnapConfiguration(AbstractView *view);
    ~SnapConfiguration();

    Q_INVOKABLE void cancel();
    Q_INVOKABLE void apply();

    void showConfigDialog(const QPoint &pos);

    void setPosInt(double value);
    double posInt() const { return m_positionInterval; }
    void setRotInt(double value);
    double rotInt() const { return m_rotationInterval; }
    void setScaleInt(double value);
    double scaleInt() const { return m_scaleInterval; }

signals:
    void posIntChanged();
    void rotIntChanged();
    void scaleIntChanged();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void cleanup();

    QPointer<QQuickView> m_configDialog;
    QPointer<AbstractView> m_view;
    double m_positionInterval = 10.;
    double m_rotationInterval = 15.;
    double m_scaleInterval = 10.;
};

} // namespace QmlDesigner
