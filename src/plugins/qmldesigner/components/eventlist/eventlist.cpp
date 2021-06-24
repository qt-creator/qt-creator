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
#include "eventlist.h"
#include "eventlistpluginview.h"
#include "eventlistview.h"
#include "nodelistview.h"

#include "bindingproperty.h"
#include "metainfo.h"
#include "projectexplorer/project.h"
#include "projectexplorer/session.h"
#include "qmldesignerplugin.h"
#include "signalhandlerproperty.h"
#include "utils/fileutils.h"
#include "utils/qtcassert.h"
#include "variantproperty.h"

#include <QDirIterator>
#include <QStandardItemModel>

namespace QmlDesigner {

Utils::FilePath projectFilePath()
{
    if (auto *doc = QmlDesignerPlugin::instance()->documentManager().currentDesignDocument()) {
        if (auto *proj = ProjectExplorer::SessionManager::projectForFile(doc->fileName()))
            return proj->projectDirectory();
    }
    return Utils::FilePath();
}

static Utils::FilePath findFile(const Utils::FilePath &path, const QString &fileName)
{
    QDirIterator it(path.toString(), QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QFileInfo file(it.next());
        if (file.isDir())
            continue;

        if (file.fileName() == fileName)
            return Utils::FilePath::fromFileInfo(file);
    }
    return {};
}


NodeListView *EventList::st_nodeView = nullptr;

void EventList::setNodeProperties(AbstractView *view)
{
    st_nodeView = new NodeListView(view);
}

void EventList::selectNode(int internalId)
{
    if (st_nodeView)
        st_nodeView->selectNode(internalId);
}

int EventList::currentNode()
{
    if (st_nodeView)
        return st_nodeView->currentNode();

    return -1;
}

bool EventList::hasEventListModel()
{
    Utils::FilePath projectPath = projectFilePath();
    if (projectPath.isEmpty())
        return false;

    Utils::FilePath path = findFile(projectPath, "EventListModel.qml");
    return path.exists();
}

void EventList::addEventIdToCurrent(const QString &eventId)
{
    int iid = currentNode();
    if (st_nodeView && iid >= 0)
        st_nodeView->addEventId(iid, eventId);
}

void EventList::removeEventIdFromCurrent(const QString &eventId)
{
    int iid = currentNode();
    if (st_nodeView && iid >= 0)
        st_nodeView->removeEventIds(iid, {eventId});
}

QString EventList::setNodeId(int internalId, const QString &id)
{
    if (st_nodeView)
        return st_nodeView->setNodeId(internalId, id);

    return QString();
}

QStandardItemModel *EventList::nodeModel()
{
    if (st_nodeView)
        return st_nodeView->itemModel();

    return nullptr;
}

NodeListView *EventList::nodeListView()
{
    return st_nodeView;
}

ModelNode EventList::modelNode(const QString &id)
{
    if (st_nodeView)
        return st_nodeView->modelNodeForId(id);

    return ModelNode();
}

void EventList::setSignalSource(SignalHandlerProperty &prop, const QString &source)
{
    if (st_nodeView) {
        QmlDesigner::Import import =
            QmlDesigner::Import::createLibraryImport("QtQuick.Studio.EventSystem", "1.0");

        if (!st_nodeView->model()->hasImport(import, true, true)) {
            try {
                st_nodeView->model()->changeImports({import}, {});
            } catch (const QmlDesigner::Exception &) {
                QTC_ASSERT(false, return );
            }
        }

        if (source == "{}") {
            if (ModelNode node = prop.parentModelNode(); node.isValid()) {
                st_nodeView->executeInTransaction("EventList::removeProperty",
                    [&]() { node.removeProperty(prop.name()); });
            }
        }
        else {
            st_nodeView->executeInTransaction("EventList::setSource",
                [&]() { prop.setSource(source); });
        }
    }
}

EventList::EventList()
    : m_model(nullptr)
    , m_eventView(nullptr)
    , m_path()

{}

Model *EventList::model() const
{
    return m_model;
}

EventListView *EventList::view() const
{
    return m_eventView;
}

QString EventList::read() const
{
    if (!m_path.exists())
        return QString();

    Utils::FileReader reader;
    QTC_ASSERT(reader.fetch(m_path), return QString());

    return QString::fromUtf8(reader.data());
}


void EventList::initialize(EventListPluginView *parent)
{
    Utils::FilePath projectPath = projectFilePath();
    QTC_ASSERT(!projectPath.isEmpty(), return );
    m_path = findFile(projectPath, "EventListModel.qml");

    if (!m_model) {
        QByteArray unqualifiedTypeName = "ListModel";
        auto metaInfo = parent->model()->metaInfo(unqualifiedTypeName);

        QByteArray fullTypeName = metaInfo.typeName();
        int minorVersion = metaInfo.minorVersion();
        int majorVersion = metaInfo.majorVersion();

        m_model = Model::create(fullTypeName, majorVersion, minorVersion);
        m_model->setParent(parent);
    }

    if (!m_eventView) {
        m_eventView = new EventListView(m_model);
        m_model->attachView(m_eventView);
    }
}

void EventList::write(const QString &text)
{
    if (!m_path.exists())
        return;

    Utils::FileSaver writer(m_path);
    writer.write(text.toUtf8());
    writer.finalize();
}

} // namespace QmlDesigner.
