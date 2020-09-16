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

#include "imagecontainer.h"

#include <vector>

namespace QmlDesigner {

template<typename Type>
QDataStream &operator<<(QDataStream &out, const std::vector<Type> &vector)
{
    out << quint64(vector.size());

    for (auto &&entry : vector)
        out << entry;

    return out;
}

template<typename Type>
QDataStream &operator>>(QDataStream &in, std::vector<Type> &vector)
{
    vector.clear();

    quint64 size;

    in >> size;

    vector.reserve(size);

    for (quint64 i = 0; i < size; ++i) {
        Type entry;

        in >> entry;

        vector.push_back(std::move(entry));
    }

    return in;
}

class CapturedDataCommand
{
public:
    struct Property
    {
        Property() = default;
        Property(QString key, QVariant value)
            : key(std::move(key))
            , value(std::move(value))
        {}

        friend QDataStream &operator<<(QDataStream &out, const Property &property)
        {
            out << property.key;
            out << property.value;

            return out;
        }

        friend QDataStream &operator>>(QDataStream &in, Property &property)
        {
            in >> property.key;
            in >> property.value;

            return in;
        }

        QString key;
        QVariant value;
    };

    struct NodeData
    {
        friend QDataStream &operator<<(QDataStream &out, const NodeData &data)
        {
            out << data.nodeId;
            out << data.contentRect;
            out << data.sceneTransform;
            out << data.properties;

            return out;
        }

        friend QDataStream &operator>>(QDataStream &in, NodeData &data)
        {
            in >> data.nodeId;
            in >> data.contentRect;
            in >> data.sceneTransform;
            in >> data.properties;

            return in;
        }

        qint32 nodeId = -1;
        QRectF contentRect;
        QTransform sceneTransform;
        std::vector<Property> properties;
    };

    struct StateData
    {
        friend QDataStream &operator<<(QDataStream &out, const StateData &data)
        {
            out << data.image;
            out << data.nodeData;
            out << data.nodeId;

            return out;
        }

        friend QDataStream &operator>>(QDataStream &in, StateData &data)
        {
            in >> data.image;
            in >> data.nodeData;
            in >> data.nodeId;

            return in;
        }

        ImageContainer image;
        std::vector<NodeData> nodeData;
        qint32 nodeId = -1;
    };

    CapturedDataCommand() = default;

    CapturedDataCommand(QVector<StateData> &&stateData)
        : stateData{std::move(stateData)}
    {}

    CapturedDataCommand(QImage &&image)
        : image{std::move(image)}
    {}

    friend QDataStream &operator<<(QDataStream &out, const CapturedDataCommand &command)
    {
        out << command.image;
        out << command.stateData;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, CapturedDataCommand &command)
    {
        in >> command.image;
        in >> command.stateData;

        return in;
    }

public:
    QImage image;
    QVector<StateData> stateData;
};

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::CapturedDataCommand)
