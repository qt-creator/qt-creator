/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Event List module.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
******************************************************************************/

#include "eventlistactions.h"
#include "eventlist.h"
#include <theme.h>

#include "metainfo.h"
#include <utils/utilsicons.h>

#include <utils/stylehelper.h>

namespace QmlDesigner {

inline bool eventListEnabled(const SelectionContext &)
{
    return EventList::hasEventListModel();
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
