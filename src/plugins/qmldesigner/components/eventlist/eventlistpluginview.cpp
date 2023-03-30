// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "eventlistpluginview.h"
#include "assigneventdialog.h"
#include "connectsignaldialog.h"
#include "designericons.h"
#include "eventlistactions.h"
#include "eventlistdialog.h"

#include "signalhandlerproperty.h"
#include <componentcore/componentcore_constants.h>
#include <coreplugin/icore.h>
#include <designeractionmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <qmldesignerplugin.h>

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

EventListPluginView::EventListPluginView(ExternalDependenciesInterface &externalDepoendencies)
    : AbstractView{externalDepoendencies}
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
                                                            designerActionManager.contextIcon(DesignerIcons::EventListIcon),
                                                            ComponentCoreConstants::Priorities::EventListCategory,
                                                            &SelectionContextFunctors::always,
                                                            &SelectionContextFunctors::always));
    auto eventListAction = new EventListAction();
    connect(eventListAction->action(), &QAction::triggered, [this]() {
        if (!m_eventListDialog)
            m_eventListDialog = new EventListDialog(Core::ICore::dialogParent());

        m_eventlist.initialize(this);
        m_eventListDialog->initialize(m_eventlist);
        m_eventListDialog->show();
    });
    designerActionManager.addDesignerAction(eventListAction);

    auto assignEventAction = new AssignEventEditorAction();
    connect(assignEventAction->action(), &QAction::triggered, [this]() {
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

    connect(connectSignalAction->action(), &QAction::triggered, [this, connectSignalAction]() {
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
