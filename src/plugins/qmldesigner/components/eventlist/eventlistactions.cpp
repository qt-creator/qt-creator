/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include "eventlistactions.h"
#include "eventlist.h"
#include <theme.h>

#include "metainfo.h"
#include <utils/utilsicons.h>

#include <utils/stylehelper.h>

namespace QmlDesigner {

inline bool eventListEnabled(const SelectionContext &context)
{
    static ModelNode lastRootNode;
    static bool lastValue = false;

    if (lastRootNode == context.rootNode())
        return lastValue;

    lastRootNode = context.rootNode();
    lastValue = EventList::hasEventListModel();

    return lastValue;
}

QIcon eventListIconFromIconFont(Theme::Icon iconType)
{
    const QColor enabledColor(Theme::getColor(Theme::IconsBaseColor));
    const QColor disabledColor(Theme::getColor(Theme::IconsDisabledColor));
    const QString unicode = Theme::getIconUnicode(iconType);
    const QString fontName = "qtds_propertyIconFont.ttf";

    const auto enabledHelper = Utils::StyleHelper::IconFontHelper(
        unicode, enabledColor, QSize(28, 28), QIcon::Normal);

    const auto disabledHelper = Utils::StyleHelper::IconFontHelper(
        unicode, disabledColor, QSize(28, 28), QIcon::Disabled);

    return Utils::StyleHelper::getIconFromIconFont(fontName, {enabledHelper, disabledHelper});
}

static QIcon editEventListIcon()
{
    return eventListIconFromIconFont(Theme::Icon::edit);
}

static void handleAction(const SelectionContext &) {}

EventListAction::EventListAction()
    : ModelNodeAction("EventList",
                      QObject::tr("Show Event List"),
                      editEventListIcon(),
                      QObject::tr("Show Event List"),
                      ComponentCoreConstants::eventListCategory,
                      QKeySequence("Alt+e"),
                      230,
                      &handleAction,
                      &eventListEnabled)
{}

static QIcon assignEventListIcon()
{
    return eventListIconFromIconFont(Theme::Icon::assign);
}

static void handleAssignEventActionOperation(const SelectionContext &context)
{
    EventList::setNodeProperties(context.view());
}

AssignEventEditorAction::AssignEventEditorAction()
    : ModelNodeAction("AssignEventEditor",
                      QObject::tr("Assign Events to Actions"),
                      assignEventListIcon(),
                      QObject::tr("Assign Events to Actions"),
                      ComponentCoreConstants::eventListCategory,
                      QKeySequence("Alt+a"),
                      220,
                      &handleAssignEventActionOperation,
                      &eventListEnabled)
{}

ConnectSignalAction::ConnectSignalAction()
    : ModelNodeContextMenuAction("ConnectSignalEditor",
                                 QObject::tr("Connect Signal to Event"),
                                 assignEventListIcon(),
                                 ComponentCoreConstants::eventListCategory,
                                 QKeySequence(),
                                 210,
                                 &handleAssignEventActionOperation)
{}

ModelNodeContextMenuAction::TargetView ConnectSignalAction::targetView() const
{
    return TargetView::ConnectionEditor;
}

} // namespace QmlDesigner
