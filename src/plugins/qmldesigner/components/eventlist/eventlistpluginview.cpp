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
#include "eventlistpluginview.h"
#include "assigneventdialog.h"
#include "connectsignaldialog.h"
#include "eventlistactions.h"
#include "eventlistdialog.h"

#include "signalhandlerproperty.h"
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <qmldesignerplugin.h>
#include <componentcore/componentcore_constants.h>

namespace QmlDesigner {

SignalHandlerProperty signalPropertyFromAction(ActionInterface *interface)
{
    if (auto data = interface->action()->data(); data.isValid()) {
        QMap<QString, QVariant> mapping = data.value<QMap<QString, QVariant>>();
        ModelNode node = EventList::modelNode(mapping["ModelNode"].toString());
        PropertyName signal = mapping["Signal"].toString().toUtf8();

        for (auto &&child : node.directSubModelNodes()) {
            if (auto prop = child.signalHandlerProperty(signal); prop.exists())
                return prop;
        }
    }
    return SignalHandlerProperty();
}

EventListPluginView::EventListPluginView(QObject* parent)
    : AbstractView(parent)
    , m_eventlist()
    , m_eventListDialog(nullptr)
    , m_assigner(nullptr)
    , m_signalConnector(nullptr)
{ }

void EventListPluginView::registerActions()
{
    DesignerActionManager &designerActionManager = QmlDesignerPlugin::instance()->designerActionManager();

    designerActionManager.addDesignerAction(new ActionGroup(tr("Event List"),
                                                            ComponentCoreConstants::eventListCategory,
                                                            ComponentCoreConstants::priorityEventListCategory,
                                                            &SelectionContextFunctors::always,
                                                            &SelectionContextFunctors::always));
    auto eventListAction = new EventListAction();
    connect(eventListAction->defaultAction(), &QAction::triggered, [this]() {
        if (!m_eventListDialog)
            m_eventListDialog = new EventListDialog(Core::ICore::dialogParent());

        m_eventlist.initialize(this);
        m_eventListDialog->initialize(m_eventlist);
        m_eventListDialog->show();
    });
    designerActionManager.addDesignerAction(eventListAction);

    auto assignEventAction = new AssignEventEditorAction();
    connect(assignEventAction->defaultAction(), &QAction::triggered, [this]() {
        if (!m_assigner)
            m_assigner = new AssignEventDialog(Core::ICore::dialogParent());
        if (!m_eventListDialog)
            m_eventListDialog = new EventListDialog(Core::ICore::dialogParent());

        m_eventlist.initialize(this);
        m_eventListDialog->initialize(m_eventlist);
        m_assigner->initialize(m_eventlist);
        m_assigner->show();
        m_assigner->postShow();
    });
    designerActionManager.addDesignerAction(assignEventAction);

    auto *connectSignalAction = new ConnectSignalAction();

    connect(connectSignalAction->defaultAction(), &QAction::triggered, [this, connectSignalAction]() {
        if (!m_signalConnector)
            m_signalConnector = new ConnectSignalDialog(Core::ICore::dialogParent());

        if (!m_eventListDialog)
            m_eventListDialog = new EventListDialog(Core::ICore::dialogParent());

        m_eventlist.initialize(this);
        m_eventListDialog->initialize(m_eventlist);

        if (auto signal = signalPropertyFromAction(connectSignalAction); signal.isValid()) {
            m_signalConnector->initialize(m_eventlist, signal);
            m_signalConnector->show();
        }
    });
    designerActionManager.addDesignerAction(connectSignalAction);
}

} // namespace QmlDesigner
