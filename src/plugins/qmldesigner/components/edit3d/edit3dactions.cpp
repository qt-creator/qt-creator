// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "edit3dactions.h"

#include "bakelights.h"
#include "edit3dview.h"
#include "nodemetainfo.h"
#include "qmldesignerconstants.h"
#include "seekerslider.h"

#include <utils/algorithm.h>

namespace QmlDesigner {

Edit3DActionTemplate::Edit3DActionTemplate(const QString &description,
                                           SelectionContextOperation action,
                                           Edit3DView *view,
                                           View3DActionType type)
    : DefaultAction(description)
    , m_action(action)
    , m_view(view)
    , m_type(type)
{}

void Edit3DActionTemplate::actionTriggered(bool b)
{
    if (m_type != View3DActionType::Empty)
        m_view->emitView3DAction(m_type, b);

    if (m_action)
        m_action(m_selectionContext);
}

Edit3DWidgetActionTemplate::Edit3DWidgetActionTemplate(QWidgetAction *widget)
    : PureActionInterface(widget)
{

}

Edit3DAction::Edit3DAction(const QByteArray &menuId,
                           View3DActionType type,
                           const QString &description,
                           const QKeySequence &key,
                           bool checkable,
                           bool checked,
                           const QIcon &icon,
                           Edit3DView *view,
                           SelectionContextOperation selectionAction,
                           const QString &toolTip)
    : Edit3DAction(menuId, type, view, new Edit3DActionTemplate(description,
                                                                selectionAction,
                                                                view,
                                                                type))
{
    action()->setShortcut(key);
    action()->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    action()->setCheckable(checkable);
    action()->setChecked(checked);

    // Description will be used as tooltip by default if no explicit tooltip is provided
    if (!toolTip.isEmpty())
        action()->setToolTip(toolTip);

    action()->setIcon(icon);
}

Edit3DAction::Edit3DAction(const QByteArray &menuId,
                           View3DActionType type,
                           Edit3DView *view,
                           PureActionInterface *pureInt)
    : AbstractAction(pureInt)
    , m_menuId(menuId)
    , m_actionType(type)
{
    view->registerEdit3DAction(this);
}

QByteArray Edit3DAction::category() const
{
    return QByteArray();
}

View3DActionType Edit3DAction::actionType() const
{
    return m_actionType;
}

bool Edit3DAction::isVisible([[maybe_unused]] const SelectionContext &selectionContext) const
{
    return true;
}

bool Edit3DAction::isEnabled(const SelectionContext &selectionContext) const
{
    return isVisible(selectionContext);
}

Edit3DParticleSeekerAction::Edit3DParticleSeekerAction(const QByteArray &menuId,
                                                       View3DActionType type,
                                                       Edit3DView *view)
    : Edit3DAction(menuId,
                   type,
                   view,
                   new Edit3DWidgetActionTemplate(
                       new SeekerSliderAction(nullptr)))
{
    m_seeker = qobject_cast<SeekerSliderAction *>(action());
}

SeekerSliderAction *Edit3DParticleSeekerAction::seekerAction()
{
    return m_seeker;
}

bool Edit3DParticleSeekerAction::isVisible(const SelectionContext &) const
{
    return m_seeker->isVisible();
}

bool Edit3DParticleSeekerAction::isEnabled(const SelectionContext &) const
{
    return m_seeker->isEnabled();
}

Edit3DBakeLightsAction::Edit3DBakeLightsAction(const QIcon &icon,
                                               Edit3DView *view,
                                               SelectionContextOperation selectionAction)
    : Edit3DAction(QmlDesigner::Constants::EDIT3D_BAKE_LIGHTS,
                   View3DActionType::Empty,
                   QCoreApplication::translate("BakeLights", "Bake Lights"),
                   QKeySequence(),
                   false,
                   false,
                   icon,
                   view,
                   selectionAction,
                   QCoreApplication::translate("BakeLights", "Bake lights for the current 3D scene."))
    , m_view(view)
{

}

bool Edit3DBakeLightsAction::isVisible(const SelectionContext &) const
{
    return m_view->isBakingLightsSupported();
}

bool Edit3DBakeLightsAction::isEnabled(const SelectionContext &) const
{
    return m_view->isBakingLightsSupported()
            && !BakeLights::resolveView3dId(m_view).isEmpty();
}

}
