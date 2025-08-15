// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "multipropertyeditoraction.h"

#include <designmodewidget.h>
#include <propertyeditorview.h>
#include <propertyeditorwidget.h>
#include <qmldesignerplugin.h>

#include <utils/qtcassert.h>

namespace QmlDesigner {

static constexpr char mainPropertyEditorId[] = "Properties";

/*!
 * \class QmlDesigner::MultiPropertyEditorAction
 * \inheaderfile multipropertyeditoraction.h
 * \internal
 * \brief This action controls or reflects the collective state of all other individual actions.
 * When activated, it enables all associated actions; when deactivated, it disables them.
 * Conversely, its state automatically updates based on whether any individual action is currently enabled.
 * The action is specialized to be used for PropertyEditorView and its action.
 */

static bool isViewClosed(AbstractView *view)
{
    if (!view->hasWidget())
        return true;
    if (auto dockManager = QmlDesignerPlugin::dockManagerForPluginInitializationOnly()) {
        if (auto dockWidget = dockManager->findDockWidget(view->widgetInfo().uniqueId))
            return dockWidget->isClosed();
    }
    return true;
}

MultiPropertyEditorAction::MultiPropertyEditorAction(QObject *parent)
    : QAction("Properties", parent)
{
    connect(this, &QAction::toggled, this, &MultiPropertyEditorAction::checkAll);
    setCheckable(true);
}

void MultiPropertyEditorAction::registerView(PropertyEditorView *view)
{
    QTC_ASSERT(view, return);

    if (m_viewsStatus.contains(view))
        return;

    bool actionIsChecked = view->action()->isChecked();
    m_viewsStatus.insert(view, actionIsChecked);
    m_views.append(view);
    updateViewsCount();

    auto widget = view->widgetInfo().widget;
    widget->installEventFilter(this);

    view->setUnifiedAction(this);

    if (actionIsChecked)
        increaseOne();

    connect(view->action(), &QAction::toggled, this, [this, view](bool value) {
        if (m_viewsStatus.value(view) == value)
            return;

        m_viewsStatus.insert(view, value);
        if (value)
            increaseOne();
        else
            decreaseOne();
    });
}

void MultiPropertyEditorAction::unregisterView(PropertyEditorView *view)
{
    QTC_ASSERT(view, return);

    if (!m_viewsStatus.contains(view))
        return;

    m_views.removeOne(view);
    updateViewsCount();

    auto widget = view->widgetInfo().widget;
    widget->removeEventFilter(this);

    const bool wasChecked = m_viewsStatus.take(view);
    if (wasChecked)
        decreaseOne();

    disconnect(view->action(), 0, this, 0);
}

bool MultiPropertyEditorAction::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
    case QEvent::Hide:
        uncheckIfAllWidgetsHidden();
        break;
    case QEvent::Show:
        setCheckedIfWidgetRegistered(watched);
        break;
    default:
        break;
    }
    return QAction::eventFilter(watched, event);
}

QList<PropertyEditorView *> MultiPropertyEditorAction::registeredViews() const
{
    return m_views;
}

void MultiPropertyEditorAction::increaseOne()
{
    m_count++;
    setChecked(true);
}

void MultiPropertyEditorAction::decreaseOne()
{
    if (--m_count == 0)
        setChecked(false);
}

void MultiPropertyEditorAction::checkAll(bool value)
{
    for (PropertyEditorView *view : std::as_const(m_views)) {
        bool currentValue = view->action()->isChecked();
        if (currentValue != value) {
            view->action()->setChecked(value);
            emit view->action()->triggered(value);

            if (value) {
                ensureParentId(view);
                view->showExtraWidget();
            } else {
                view->closeExtraWidget();
            }
        }
    }

    QmlDesignerPlugin::instance()->viewManager().hideSingleWidgetTitleBars(mainPropertyEditorId);
}

void MultiPropertyEditorAction::uncheckIfAllWidgetsHidden()
{
    if (!isChecked())
        return;

    for (PropertyEditorView *view : std::as_const(m_views)) {
        if (!isViewClosed(view))
            return;
    }
    setChecked(false);
}

void MultiPropertyEditorAction::setCheckedIfWidgetRegistered(QObject *widgetObject)
{
    if (isChecked())
        return;

    for (PropertyEditorView *view : std::as_const(m_views)) {
        if (auto w = view->widgetInfo().widget; w == widgetObject) {
            setChecked(true);
            return;
        }
    }
}

void MultiPropertyEditorAction::ensureParentId(PropertyEditorView *view)
{
    if (!view || view == QmlDesignerPlugin::viewManager().propertyEditorView())
        return;

    auto widgetInfo = view->widgetInfo();
    auto parentView = QmlDesignerPlugin::viewManager().findView(widgetInfo.parentId);
    if (parentView)
        return;

    widgetInfo.parentId = mainPropertyEditorId;
    view->setWidgetInfo(widgetInfo);
}

void MultiPropertyEditorAction::updateViewsCount()
{
    const int instancesCount = m_views.count();
    for (PropertyEditorView *view : std::as_const(m_views))
        view->setInstancesCount(instancesCount);
}

} // namespace QmlDesigner
