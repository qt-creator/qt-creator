// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtCore/qtconfigmacros.h>

QT_BEGIN_NAMESPACE
class QJsonArray;
class QUrl;
QT_END_NAMESPACE

namespace QmlDesigner::CollectionEditor::ImportTools {

QJsonArray loadAsSingleJsonCollection(const QUrl &url);
QJsonArray loadAsCsvCollection(const QUrl &url);

} // namespace QmlDesigner::CollectionEditor::ImportTools
