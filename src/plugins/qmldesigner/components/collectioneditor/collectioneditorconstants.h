// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QmlDesigner::CollectionEditor {

enum class SourceFormat { Unknown, Json, Csv };

inline constexpr char SOURCEFILE_PROPERTY[]             = "source";

inline constexpr char COLLECTIONMODEL_IMPORT[]          = "QtQuick.Studio.Utils";
inline constexpr char JSONCOLLECTIONMODEL_TYPENAME[]    = "QtQuick.Studio.Utils.JsonListModel";
inline constexpr char CSVCOLLECTIONMODEL_TYPENAME[]     = "QtQuick.Studio.Utils.CsvTableModel";

} // namespace QmlDesigner::CollectionEditor
