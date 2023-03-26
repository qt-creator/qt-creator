// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "formeditoritem.h"

#include <QRectF>

namespace QmlDesigner {

QRectF boundingRectForItemList(QList<FormEditorItem*> itemList);

ModelNode parentNodeSemanticallyChecked(const ModelNode &childNode, const ModelNode &parentNode);

FormEditorItem* mapGraphicsViewToGraphicsScene(FormEditorItem* item);

} // namespace QmlDesigner
