/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
