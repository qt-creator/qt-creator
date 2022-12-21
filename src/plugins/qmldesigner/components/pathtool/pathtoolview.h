// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>

namespace QmlDesigner {

class PathTool;

class PathToolView : public AbstractView
{
    Q_OBJECT
public:
    PathToolView(PathTool *pathTool, ExternalDependenciesInterface &externalDependencies);

    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange) override;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) override;

private:
    PathTool *m_pathTool;
};

} // namespace QmlDesigner
