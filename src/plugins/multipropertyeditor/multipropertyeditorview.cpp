// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "multipropertyeditorview.h"

#include "multipropertyeditoraction.h"

#include <propertyeditorview.h>
#include <qmldesignerplugin.h>

#include <uniquename.h>
#include <utils/algorithm.h>
namespace QmlDesigner {

MultiPropertyEditorView::MultiPropertyEditorView(
    class AsynchronousImageCache &imageCache, ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_imageCache{imageCache}
    , m_externalDependencies{externalDependencies}
    , m_unifiedAction(new MultiPropertyEditorAction(this))
{}

MultiPropertyEditorView::~MultiPropertyEditorView() {};

void MultiPropertyEditorView::customNotification(const AbstractView *view,
                                                 const QString &identifier,
                                                 const QList<ModelNode> &nodeList,
                                                 const QList<QVariant> &data)
{
    auto &viewManager = QmlDesignerPlugin::instance()->viewManager();

    if (identifier == "add_extra_property_editor_widget") {
        auto newView = std::make_unique<PropertyEditorView>(m_imageCache, m_externalDependencies);
        WidgetInfo info = newView->widgetInfo();
        info.tabName = tr("Properties");
        info.parentId = data.at(0).toString();
        newView->setWidgetInfo(info);
        PropertyEditorView *propertyEditorView = viewManager.registerView(std::move(newView));
        propertyEditorView->setUnifiedAction(unifiedAction());
        unifiedAction()->registerView(propertyEditorView);
        propertyEditorView->registerWidgetInfo();
        propertyEditorView->action()->setChecked(true);
        viewManager.showView(*propertyEditorView);
    } else if (identifier == "register_property_editor") {
        if (data.size() != 1)
            return;
        PropertyEditorView *propertyEditorView = data.first().value<PropertyEditorView *>();
        unifiedAction()->registerView(propertyEditorView);
    } else if (identifier == "unregister_property_editor") {
        if (data.size() != 1)
            return;
        PropertyEditorView *propertyEditorView = data.first().value<PropertyEditorView *>();
        unifiedAction()->unregisterView(propertyEditorView);
    }
}

// Controls and reflects the collective on/off state of all other individual view actions
MultiPropertyEditorAction *MultiPropertyEditorView::unifiedAction() const
{
    return m_unifiedAction;
}

} // namespace QmlDesigner
