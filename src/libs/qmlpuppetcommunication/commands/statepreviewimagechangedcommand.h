// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>

#include "imagecontainer.h"

namespace QmlDesigner {

class StatePreviewImageChangedCommand
{
    friend QDataStream &operator>>(QDataStream &in, StatePreviewImageChangedCommand &command);
    friend bool operator ==(const StatePreviewImageChangedCommand &first, const StatePreviewImageChangedCommand &second);

public:
    StatePreviewImageChangedCommand();
    explicit StatePreviewImageChangedCommand(const QVector<ImageContainer> &imageVector);

    QVector<ImageContainer> previews() const;

    void sort();

private:
    QVector<ImageContainer> m_previewVector;
};

QDataStream &operator<<(QDataStream &out, const StatePreviewImageChangedCommand &command);
QDataStream &operator>>(QDataStream &in, StatePreviewImageChangedCommand &command);

bool operator ==(const StatePreviewImageChangedCommand &first, const StatePreviewImageChangedCommand &second);
QDebug operator <<(QDebug debug, const StatePreviewImageChangedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::StatePreviewImageChangedCommand)
