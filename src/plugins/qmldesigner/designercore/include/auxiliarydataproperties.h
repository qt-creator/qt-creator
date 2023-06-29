// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <auxiliarydata.h>

#include <QColor>
#include <QVariant>

#include <type_traits>
#include <variant>

namespace QmlDesigner {

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
inline constexpr AuxiliaryDataKeyDefaultValue insightEnabledProperty{AuxiliaryDataType::Temporary,
                                                                     "insightEnabled",
                                                                     false};
inline constexpr AuxiliaryDataKeyDefaultValue insightCategoriesProperty{AuxiliaryDataType::Temporary,
                                                                        "insightCategories",
                                                                        {}};
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
inline constexpr AuxiliaryDataKeyView languageProperty{AuxiliaryDataType::Temporary, "language"};
inline constexpr AuxiliaryDataKeyView bakeLightsManualProperty{AuxiliaryDataType::Document,
                                                               "bakeLightsManual"};
inline constexpr AuxiliaryDataKeyView contextImageProperty{AuxiliaryDataType::Temporary,
                                                           "contextImage"};

// Most material preview aux properties are duplicated as document and instance types, as they
// are both required to be persistent and used at runtime to control material preview rendering
inline constexpr AuxiliaryDataKeyView materialPreviewEnvDocProperty{
    AuxiliaryDataType::Document, "matPrevEnvDoc"};
inline constexpr AuxiliaryDataKeyView materialPreviewEnvValueDocProperty{
    AuxiliaryDataType::Document, "matPrevEnvValueDoc"};
inline constexpr AuxiliaryDataKeyView materialPreviewModelDocProperty{
    AuxiliaryDataType::Document, "matPrevModelDoc"};
inline constexpr AuxiliaryDataKeyView materialPreviewColorDocProperty{
    AuxiliaryDataType::Document, "matPrevColorDoc"}; // Only needed on doc side
inline constexpr AuxiliaryDataKeyView materialPreviewEnvProperty{
    AuxiliaryDataType::NodeInstanceAuxiliary, "matPrevEnv"};
inline constexpr AuxiliaryDataKeyView materialPreviewEnvValueProperty{
    AuxiliaryDataType::NodeInstanceAuxiliary, "matPrevEnvValue"};
inline constexpr AuxiliaryDataKeyView materialPreviewModelProperty{AuxiliaryDataType::NodeInstanceAuxiliary,
                                                                   "matPrevModel"};

} // namespace QmlDesigner
