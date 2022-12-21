// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "isceditor.h"
#include "mytypes.h"

#include <QObject>

namespace ScxmlEditor {

namespace PluginInterface {

class ScxmlUiFactory;
class SCAttributeItemDelegate;
class SCAttributeItemModel;
class ScxmlDocument;
class SCGraphicsItemProvider;
class SCShapeProvider;
class SCUtilsProvider;

class GenericScxmlPlugin : public QObject, public ISCEditor
{
    Q_OBJECT
    Q_INTERFACES(ScxmlEditor::PluginInterface::ISCEditor)

public:
    GenericScxmlPlugin(QObject *parent = nullptr);
    ~GenericScxmlPlugin() override;

    void init(ScxmlUiFactory *factory) override;
    void detach() override;
    void documentChanged(DocumentChangeType type, ScxmlDocument *doc) override;
    void refresh() override;

private:
    ScxmlUiFactory *m_factory = nullptr;
    SCAttributeItemDelegate *m_attributeItemDelegate = nullptr;
    SCAttributeItemModel *m_attributeItemModel = nullptr;
    SCGraphicsItemProvider *m_graphicsItemProvider = nullptr;
    SCShapeProvider *m_shapeProvider = nullptr;
    SCUtilsProvider *m_utilsProvider = nullptr;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
