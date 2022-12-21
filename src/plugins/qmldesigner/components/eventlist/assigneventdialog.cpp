// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "assigneventdialog.h"
#include "eventlist.h"
#include "eventlistdelegate.h"
#include "eventlistview.h"
#include "filterlinewidget.h"
#include "nodelistdelegate.h"
#include "nodelistview.h"
#include "nodeselectionmodel.h"
#include "eventlistutils.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>

namespace QmlDesigner {

AssignEventDialog::AssignEventDialog(QWidget *parent)
    : QDialog(parent)
    , m_nodeTable(new QTableView)
    , m_eventTable(new QTableView)
    , m_nodeLine(new FilterLineWidget())
    , m_eventLine(new FilterLineWidget())
{
    setWindowFlag(Qt::Tool, true);
    setModal(true);

    auto *nodeFilterModel = new QSortFilterProxyModel;
    auto *nodeListDelegate = new NodeListDelegate(m_nodeTable);
    auto *nodeSelectionModel = new NodeSelectionModel(nodeFilterModel);
    m_nodeTable->installEventFilter(new TabWalker(this));
    m_nodeTable->setItemDelegate(nodeListDelegate);
    m_nodeTable->setModel(nodeFilterModel);
    m_nodeTable->setSelectionModel(nodeSelectionModel);
    m_nodeTable->setFocusPolicy(Qt::NoFocus);
    m_nodeTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_nodeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_nodeTable->resizeColumnsToContents();
    m_nodeTable->horizontalHeader()->setStretchLastSection(true);
    m_nodeTable->verticalHeader()->hide();
    polishPalette(m_nodeTable, "#1f75cc");

    auto *eventFilterModel = new QSortFilterProxyModel;
    auto *eventListDelegate = new EventListDelegate(m_eventTable);
    m_eventTable->installEventFilter(new TabWalker(this));
    m_eventTable->setItemDelegate(eventListDelegate);
    m_eventTable->setFocusPolicy(Qt::NoFocus);
    m_eventTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_eventTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_eventTable->setModel(eventFilterModel);
    m_eventTable->verticalHeader()->hide();
    polishPalette(m_eventTable, QColor("#d87b00"));

    auto *nodeBox = new QVBoxLayout;
    nodeBox->addWidget(m_nodeLine);
    nodeBox->addWidget(m_nodeTable);

    QWidget *nodeWidget = new QWidget;
    nodeWidget->setLayout(nodeBox);

    auto *eventBox = new QVBoxLayout;
    eventBox->addWidget(m_eventLine);
    eventBox->addWidget(m_eventTable);

    auto *eventWidget = new QWidget;
    eventWidget->setLayout(eventBox);

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(nodeWidget);
    splitter->addWidget(eventWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);

    auto *box = new QHBoxLayout;
    box->addWidget(splitter);
    setLayout(box);

    connect(m_nodeLine, &FilterLineWidget::filterChanged, [this](const QString &str) {
        if (auto *sm = qobject_cast<NodeSelectionModel *>(m_nodeTable->selectionModel())) {
            sm->storeSelection();
            if (auto *fm = qobject_cast<QSortFilterProxyModel *>(m_nodeTable->model()))
                fm->setFilterFixedString(str);
            sm->reselect();
        }
    });

    connect(m_eventLine, &FilterLineWidget::filterChanged, [this](const QString &str) {
        if (auto *fm = qobject_cast<QSortFilterProxyModel *>(m_eventTable->model()))
            fm->setFilterFixedString(str);
    });

    connect(eventListDelegate, &EventListDelegate::connectClicked,
            [](const QString &id, bool connected) {
                if (connected)
                    EventList::addEventIdToCurrent(id);
                else
                    EventList::removeEventIdFromCurrent(id);
            });
}

void AssignEventDialog::initialize(EventList &events)
{
    m_nodeLine->clear();
    m_eventLine->clear();

    if (auto *fm = qobject_cast<QSortFilterProxyModel *>(m_nodeTable->model()))
        fm->setSourceModel(events.nodeModel());

    if (auto *fm = qobject_cast<QSortFilterProxyModel *>(m_eventTable->model()))
        fm->setSourceModel(events.view()->eventListModel());

    if (auto *sm = qobject_cast<NodeSelectionModel *>(m_nodeTable->selectionModel())) {
        if (m_connection)
            sm->disconnect(m_connection);

        auto updateEventListView = [this, &events](const QStringList &eventIds) {
            auto nonExistent = events.view()->eventListModel()->connectEvents(eventIds);

            if (!nonExistent.empty()) {
                QString header(tr("Nonexistent events discovered"));
                QString msg(tr("The Node references the following nonexistent events:\n"));
                for (auto &&id : nonExistent)
                    msg += id + ", ";
                msg.remove(msg.size() - 2, 2);
                msg += "\nDo you want to remove these references?";

                if (QMessageBox::Yes == QMessageBox::question(this, header, msg)) {
                    auto *view = events.nodeListView();
                    view->removeEventIds(view->currentNode(), nonExistent);
                    view->reset();
                    if (auto *sm = qobject_cast<NodeSelectionModel *>(m_nodeTable->selectionModel()))
                        sm->selectNode(view->currentNode());
                }
            }
            m_eventTable->update();
        };
        m_connection = connect(sm, &NodeSelectionModel::nodeSelected, updateEventListView);
    }

    m_nodeTable->setColumnHidden(NodeListModel::typeColumn, true);
    m_nodeTable->setColumnHidden(NodeListModel::fromColumn, true);
    m_nodeTable->setColumnHidden(NodeListModel::toColumn, true);

    if (QHeaderView *header = m_eventTable->horizontalHeader()) {
        header->setSectionResizeMode(EventListModel::idColumn, QHeaderView::Stretch);
        header->setSectionResizeMode(EventListModel::descriptionColumn, QHeaderView::Stretch);
        header->setSectionResizeMode(EventListModel::shortcutColumn, QHeaderView::Stretch);
        header->resizeSection(EventListModel::connectColumn, 120);
        header->setStretchLastSection(false);
    }
}

void AssignEventDialog::postShow()
{
    if (auto *sm = qobject_cast<NodeSelectionModel *>(m_nodeTable->selectionModel()))
        sm->selectNode(EventList::currentNode());

    resize(QSize(700, 300));
}

} // namespace QmlDesigner.
