// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extrapropertyeditoraction.h"

#include <designmodewidget.h>
#include <propertyeditorview.h>
#include <propertyeditorwidget.h>
#include <qmldesignerplugin.h>

#include <utils/qtcassert.h>

namespace QmlDesigner {

static constexpr char mainPropertyEditorId[] = "Properties";

/*!
 * \class QmlDesigner::ExtraPropertyEditorAction
 * \inheaderfile extrapropertyeditoraction.h
 * \internal
 * \brief This action controls or reflects the collective state of all other individual actions.
 * When activated, it enables all associated actions; when deactivated, it disables them.
 * Conversely, its state automatically updates based on whether any individual action is currently enabled.
 * The action is specialized to be used for PropertyEditorView and its action.
 */

ExtraPropertyEditorAction::ExtraPropertyEditorAction(QObject *parent)
    : QAction("Properties", parent)
{
    connect(this, &QAction::toggled, this, &ExtraPropertyEditorAction::checkAll);
    setCheckable(true);
}

void ExtraPropertyEditorAction::registerView(PropertyEditorView *view)
{
    QTC_ASSERT(view, return);

    if (m_viewsStatus.contains(view))
        return;

    bool actionIsChecked = view->action()->isChecked();
    m_viewsStatus.insert(view, actionIsChecked);
    m_views.append(view);

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

void ExtraPropertyEditorAction::unregisterView(PropertyEditorView *view)
{
    QTC_ASSERT(view, return);

    if (!m_viewsStatus.contains(view))
        return;

    m_views.removeOne(view);

    auto widget = view->widgetInfo().widget;
    widget->removeEventFilter(this);

    const bool wasChecked = m_viewsStatus.take(view);
    if (wasChecked)
        decreaseOne();

    disconnect(view->action(), 0, this, 0);
}

bool ExtraPropertyEditorAction::eventFilter(QObject *watched, QEvent *event)
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

void ExtraPropertyEditorAction::increaseOne()
{
    m_count++;
    setChecked(true);
}

void ExtraPropertyEditorAction::decreaseOne()
{
    if (--m_count == 0)
        setChecked(false);
}

void ExtraPropertyEditorAction::checkAll(bool value)
{
    for (PropertyEditorView *view : std::as_const(m_views)) {
        bool currentValue = view->action()->isChecked();
        if (currentValue != value) {
            view->action()->setChecked(value);
            emit view->action()->triggered(value);

            if (value) {
                ensureParentId(view);
                showExtraWidget(view);
            } else {
                closeExtraWidget(view);
            }
        }
    }

    QmlDesignerPlugin::instance()->viewManager().hideSingleWidgetTitleBars(mainPropertyEditorId);
}

void ExtraPropertyEditorAction::uncheckIfAllWidgetsHidden()
{
    if (!isChecked())
        return;

    for (PropertyEditorView *view : std::as_const(m_views)) {
        if (auto w = view->widgetInfo().widget; w && w->isVisible())
            return;
    }
    setChecked(false);
}

void ExtraPropertyEditorAction::setCheckedIfWidgetRegistered(QObject *widgetObject)
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

void ExtraPropertyEditorAction::ensureParentId(PropertyEditorView *view)
{
    if (!view || view == PropertyEditorView::instance())
        return;

    auto widgetInfo = view->widgetInfo();
    auto parentView = QmlDesignerPlugin::viewManager().findView(widgetInfo.parentId);
    if (parentView)
        return;

    widgetInfo.parentId = mainPropertyEditorId;
    view->setWidgetInfo(widgetInfo);
}

void ExtraPropertyEditorAction::showExtraWidget(PropertyEditorView *view)
{
    if (auto wr = view->widgetRegistration())
        wr->showExtraWidget(view->widgetInfo());
}

void ExtraPropertyEditorAction::closeExtraWidget(PropertyEditorView *view)
{
    if (auto wr = view->widgetRegistration())
        wr->hideExtraWidget(view->widgetInfo());
}

} // namespace QmlDesigner
