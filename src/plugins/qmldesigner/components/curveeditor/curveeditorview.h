// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "curveeditor.h"
#include "curveeditormodel.h"
#include <abstractview.h>

namespace QmlDesigner {

class TimelineWidget;

class CurveEditorView : public AbstractView
{
    Q_OBJECT

public:
    explicit CurveEditorView(ExternalDependenciesInterface &externalDepoendencies);
    ~CurveEditorView() override;

public:
    bool hasWidget() const override;

    WidgetInfo widgetInfo() override;

    void modelAttached(Model *model) override;

    void modelAboutToBeDetached(Model *model) override;

    void nodeRemoved(const ModelNode &removedNode,
                     const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) override;

    void nodeReparented(const ModelNode &node,
                        const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        PropertyChangeFlags propertyChange) override;

    void auxiliaryDataChanged(const ModelNode &node,
                              AuxiliaryDataKeyView key,
                              const QVariant &data) override;

    void instancePropertyChanged(const QList<QPair<ModelNode, PropertyName>> &propertyList) override;

    void variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override;

    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override;

    void propertiesRemoved(const QList<AbstractProperty> &propertyList) override;

private:
    QmlTimeline activeTimeline() const;

    void updateKeyframes();
    void updateCurrentFrame(const ModelNode &node);
    void updateStartFrame(const ModelNode &node);
    void updateEndFrame(const ModelNode &node);

    void commitKeyframes(TreeItem *item);
    void commitCurrentFrame(int frame);
    void commitStartFrame(int frame);
    void commitEndFrame(int frame);
    void init();

private:
    bool m_block;
    CurveEditorModel *m_model;
    CurveEditor *m_editor;
};

} // namespace QmlDesigner
