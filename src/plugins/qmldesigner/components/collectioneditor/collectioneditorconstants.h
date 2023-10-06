// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QmlDesigner::CollectionEditor {

enum class SourceFormat { Unknown, Json, Csv };

inline constexpr char SOURCEFILE_PROPERTY[]             = "sourceFile";

inline constexpr char COLLECTIONMODEL_IMPORT[]          = "QtQuick.Studio.Models";
inline constexpr char JSONCOLLECTIONMODEL_TYPENAME[]    = "QtQuick.Studio.Models.JsonSourceModel";
inline constexpr char CSVCOLLECTIONMODEL_TYPENAME[]     = "QtQuick.Studio.Models.CsvSourceModel";

} // namespace QmlDesigner::CollectionEditor
