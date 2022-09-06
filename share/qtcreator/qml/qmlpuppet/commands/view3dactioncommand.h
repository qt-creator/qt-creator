/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
                GetNodeAtPos
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
