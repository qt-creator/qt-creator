// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QmlDesigner::CollectionEditor {

enum class SourceFormat { Unknown, Json, Csv };

inline constexpr char SOURCEFILE_PROPERTY[]                 = "source";
inline constexpr char ALLMODELS_PROPERTY[]                  = "allModels";
inline constexpr char JSONCHILDMODELNAME_PROPERTY[]         = "modelName";

inline constexpr char COLLECTIONMODEL_IMPORT[]              = "QtQuick.Studio.Utils";
inline constexpr char JSONCOLLECTIONMODEL_TYPENAME[]        = "QtQuick.Studio.Utils.JsonListModel";
inline constexpr char CSVCOLLECTIONMODEL_TYPENAME[]         = "QtQuick.Studio.Utils.CsvTableModel";
inline constexpr char JSONCOLLECTIONCHILDMODEL_TYPENAME[]   = "QtQuick.Studio.Utils.ChildListModel";
inline constexpr char JSONBACKEND_TYPENAME[]                = "JsonData";

} // namespace QmlDesigner::CollectionEditor
