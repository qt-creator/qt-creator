/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <auxiliarydata.h>
#include <utils/variant.h>

#include <QColor>
#include <QVariant>

#include <type_traits>

namespace QmlDesigner {

using PropertyValue = Utils::variant<int, long long, double, bool, QColor, QStringView, Qt::Corner>;

inline QVariant toQVariant(const PropertyValue &variant)
{
    return Utils::visit([](const auto &value) { return QVariant::fromValue(value); }, variant);
}

class AuxiliaryDataKeyDefaultValue : public AuxiliaryDataKeyView
{
public:
    constexpr AuxiliaryDataKeyDefaultValue() = default;
    constexpr AuxiliaryDataKeyDefaultValue(AuxiliaryDataType type,
                                           Utils::SmallStringView name,
                                           PropertyValue defaultValue)
        : AuxiliaryDataKeyView{type, name}
        , defaultValue{std::move(defaultValue)}
    {}

public:
    PropertyValue defaultValue;
};

inline constexpr AuxiliaryDataKeyDefaultValue customIdProperty{AuxiliaryDataType::Document,
                                                               "customId",
                                                               QStringView{}};
inline constexpr AuxiliaryDataKeyDefaultValue widthProperty{
    AuxiliaryDataType::NodeInstancePropertyOverwrite, "width", 4};
inline constexpr AuxiliaryDataKeyView heightProperty{AuxiliaryDataType::NodeInstancePropertyOverwrite,
                                                     "height"};
inline constexpr AuxiliaryDataKeyDefaultValue breakPointProperty{AuxiliaryDataType::Document,
                                                                 "breakPoint",
                                                                 50};
inline constexpr AuxiliaryDataKeyDefaultValue bezierProperty{AuxiliaryDataType::Document, "bezier", 50};
inline constexpr AuxiliaryDataKeyDefaultValue transitionBezierProperty{AuxiliaryDataType::Document,
                                                                       "transitionBezier",
                                                                       50};
inline constexpr AuxiliaryDataKeyDefaultValue typeProperty{AuxiliaryDataType::Document, "type", 0};
inline constexpr AuxiliaryDataKeyDefaultValue transitionTypeProperty{AuxiliaryDataType::Document,
                                                                     "transitionType",
                                                                     0};
inline constexpr AuxiliaryDataKeyDefaultValue radiusProperty{AuxiliaryDataType::Document, "radius", 8};
inline constexpr AuxiliaryDataKeyDefaultValue transitionRadiusProperty{AuxiliaryDataType::Document,
                                                                       "transitionRadius",
                                                                       8};
inline constexpr AuxiliaryDataKeyDefaultValue labelPositionProperty{AuxiliaryDataType::Document,
                                                                    "labelPosition",
                                                                    50.0};
inline constexpr AuxiliaryDataKeyDefaultValue labelFlipSideProperty{AuxiliaryDataType::Document,
                                                                    "labelFlipSide",
                                                                    false};
inline constexpr AuxiliaryDataKeyDefaultValue inOffsetProperty{AuxiliaryDataType::Document,
                                                               "inOffset",
                                                               0};
inline constexpr AuxiliaryDataKeyDefaultValue outOffsetProperty{AuxiliaryDataType::Document,
                                                                "outOffset",
                                                                0};
inline constexpr AuxiliaryDataKeyDefaultValue blockSizeProperty{AuxiliaryDataType::Document,
                                                                "blockSize",
                                                                200};
inline constexpr AuxiliaryDataKeyDefaultValue blockRadiusProperty{AuxiliaryDataType::Document,
                                                                  "blockRadius",
                                                                  18};
inline constexpr AuxiliaryDataKeyDefaultValue blockColorProperty{AuxiliaryDataType::Document,
                                                                 "blockColor",
                                                                 QColor{255, 0, 0}};
inline constexpr AuxiliaryDataKeyDefaultValue showDialogLabelProperty{AuxiliaryDataType::Document,
                                                                      "showDialogLabel",
                                                                      false};
inline constexpr AuxiliaryDataKeyDefaultValue dialogLabelPositionProperty{AuxiliaryDataType::Document,
                                                                          "dialogLabelPosition",
                                                                          Qt::TopRightCorner};
inline constexpr AuxiliaryDataKeyDefaultValue transitionColorProperty{AuxiliaryDataType::Document,
                                                                      "transitionColor",
                                                                      QColor{255, 0, 0}};
inline constexpr AuxiliaryDataKeyDefaultValue joinConnectionProperty{AuxiliaryDataType::Document,
                                                                     "joinConnection",
                                                                     false};
inline constexpr AuxiliaryDataKeyDefaultValue areaColorProperty{AuxiliaryDataType::Document,
                                                                "areaColor",
                                                                QColor{255, 0, 0}};
inline constexpr AuxiliaryDataKeyDefaultValue colorProperty{AuxiliaryDataType::Document,
                                                            "color",
                                                            QColor{255, 0, 0}};
inline constexpr AuxiliaryDataKeyDefaultValue dashProperty{AuxiliaryDataType::Document, "dash", false};
inline constexpr AuxiliaryDataKeyDefaultValue areaFillColorProperty{AuxiliaryDataType::Document,
                                                                    "areaFillColor",
                                                                    QColor{0, 0, 0, 0}};
inline constexpr AuxiliaryDataKeyDefaultValue fillColorProperty{AuxiliaryDataType::Document,
                                                                "fillColor",
                                                                QColor{0, 0, 0, 0}};
inline constexpr AuxiliaryDataKeyView uuidProperty{AuxiliaryDataType::Document, "uuid"};
inline constexpr AuxiliaryDataKeyView active3dSceneProperty{AuxiliaryDataType::Temporary,
                                                            "active3dScene"};
inline constexpr AuxiliaryDataKeyView tmpProperty{AuxiliaryDataType::Temporary, "tmp"};
inline constexpr AuxiliaryDataKeyView recordProperty{AuxiliaryDataType::Temporary, "Record"};
inline constexpr AuxiliaryDataKeyView transitionDurationProperty{AuxiliaryDataType::Document,
                                                                 "transitionDuration"};
inline constexpr AuxiliaryDataKeyView targetProperty{AuxiliaryDataType::Document, "target"};
inline constexpr AuxiliaryDataKeyView propertyProperty{AuxiliaryDataType::Document, "property"};
inline constexpr AuxiliaryDataKeyView currentFrameProperty{AuxiliaryDataType::NodeInstancePropertyOverwrite,
                                                           "currentFrame"};
inline constexpr AuxiliaryDataKeyView annotationProperty{AuxiliaryDataType::Document, "annotation"};
inline constexpr AuxiliaryDataKeyView globalAnnotationProperty{AuxiliaryDataType::Document,
                                                               "globalAnnotation"};
inline constexpr AuxiliaryDataKeyView globalAnnotationStatus{AuxiliaryDataType::Document,
                                                             "globalAnnotationStatus"};
inline constexpr AuxiliaryDataKeyView rotBlockProperty{AuxiliaryDataType::NodeInstanceAuxiliary,
                                                       "rotBlock"};

template<typename Type>
QVariant getDefaultValueAsQVariant(const Type &key)
{
    if constexpr (std::is_same_v<AuxiliaryDataKey, AuxiliaryDataKeyDefaultValue>)
        return toQVariant(key.defaultvalue);

    return {};
}

} // namespace QmlDesigner
