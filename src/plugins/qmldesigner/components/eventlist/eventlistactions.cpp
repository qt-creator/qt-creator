// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
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
                      11,
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
                      21,
                      &handleAssignEventActionOperation,
                      &eventListEnabled)
{}

ConnectSignalAction::ConnectSignalAction()
    : ModelNodeContextMenuAction("ConnectSignalEditor",
                                 QObject::tr("Connect Signal to Event"),
                                 assignEventListIcon(),
                                 ComponentCoreConstants::eventListCategory,
                                 QKeySequence(),
                                 31,
                                 &handleAssignEventActionOperation)
{}

ModelNodeContextMenuAction::TargetView ConnectSignalAction::targetView() const
{
    return TargetView::ConnectionEditor;
}

} // namespace QmlDesigner
