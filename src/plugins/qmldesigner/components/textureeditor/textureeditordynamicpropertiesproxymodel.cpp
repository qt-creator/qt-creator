// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textureeditordynamicpropertiesproxymodel.h"

#include "dynamicpropertiesmodel.h"
#include "textureeditorview.h"

namespace QmlDesigner {

TextureEditorDynamicPropertiesProxyModel::TextureEditorDynamicPropertiesProxyModel(QObject *parent)
    : DynamicPropertiesProxyModel(parent)
{
    if (TextureEditorView::instance())
        initModel(TextureEditorView::instance()->dynamicPropertiesModel());
}

void TextureEditorDynamicPropertiesProxyModel::registerDeclarativeType()
{
    DynamicPropertiesProxyModel::registerDeclarativeType();
    qmlRegisterType<TextureEditorDynamicPropertiesProxyModel>("HelperWidgets", 2, 0, "TextureEditorDynamicPropertiesModel");
}

} // namespace QmlDesigner
