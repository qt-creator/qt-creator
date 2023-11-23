// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "collectiondetails.h"
#include "collectioneditorconstants.h"

QT_BEGIN_NAMESPACE
class QJsonArray;
QT_END_NAMESPACE

namespace QmlDesigner::CollectionEditor {

bool variantIslessThan(const QVariant &a, const QVariant &b, CollectionDetails::DataType type);

SourceFormat getSourceCollectionFormat(const QmlDesigner::ModelNode &node);

QString getSourceCollectionType(const QmlDesigner::ModelNode &node);

QString getSourceCollectionPath(const QmlDesigner::ModelNode &dataStoreNode);

void assignCollectionSourceToNode(AbstractView *view,
                                  const ModelNode &modelNode,
                                  const ModelNode &collectionSourceNode = {});

bool canAcceptCollectionAsModel(const ModelNode &node);

QJsonArray defaultCollectionArray();

} // namespace QmlDesigner::CollectionEditor
