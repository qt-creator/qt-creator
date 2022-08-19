// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QVariant>

namespace QmlDesigner {

class View3DActionCommand
{
    friend QDataStream &operator>>(QDataStream &in, View3DActionCommand &command);
    friend QDebug operator<<(QDebug debug, const View3DActionCommand &command);

public:
    enum Type { Empty,
                MoveTool,
                ScaleTool,
                RotateTool,
                FitToView,
                AlignCamerasToView,
                AlignViewToCamera,
                SelectionModeToggle,
                CameraToggle,
                OrientationToggle,
                EditLightToggle,
                ShowGrid,
                ShowSelectionBox,
                ShowIconGizmo,
                ShowCameraFrustum,
                ShowParticleEmitter,
                Edit3DParticleModeToggle,
                ParticlesPlay,
                ParticlesRestart,
                ParticlesSeek,
                SelectBackgroundColor,
                SelectGridColor,
                ResetBackgroundColor,
                SyncBackgroundColor,
                GetModelAtPos
              };

    View3DActionCommand(Type type, const QVariant &value);

    View3DActionCommand() = default;

    bool isEnabled() const;
    QVariant value() const;
    Type type() const;
    int position() const;

private:
    Type m_type = Empty;
    QVariant m_value;

protected:
    View3DActionCommand(int pos);
};

class View3DSeekActionCommand : public View3DActionCommand
{
public:
    View3DSeekActionCommand(int pos) : View3DActionCommand(pos) {}
};

QDataStream &operator<<(QDataStream &out, const View3DActionCommand &command);
QDataStream &operator>>(QDataStream &in, View3DActionCommand &command);

QDebug operator<<(QDebug debug, const View3DActionCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::View3DActionCommand)
