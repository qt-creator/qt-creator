// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "collectiondetails.h"

namespace QmlDesigner::CollectionEditor {

bool variantIslessThan(const QVariant &a, const QVariant &b, CollectionDetails::DataType type);

QString getSourceCollectionType(const QmlDesigner::ModelNode &node);

QString getSourceCollectionPath(const QmlDesigner::ModelNode &node);

void assignCollectionSourceToNode(AbstractView *view,
                                  const ModelNode &modelNode,
                                  const ModelNode &collectionSourceNode = {});

bool canAcceptCollectionAsModel(const ModelNode &node);

} // namespace QmlDesigner::CollectionEditor
