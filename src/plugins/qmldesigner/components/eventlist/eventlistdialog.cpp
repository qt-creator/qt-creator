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
#include "eventlistdialog.h"
#include "eventlist.h"
#include "eventlistdelegate.h"
#include "eventlistview.h"
#include "filterlinewidget.h"
#include "nodelistview.h"
#include "qmldesignerplugin.h"
#include "eventlistutils.h"
#include "utils/utilsicons.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>

namespace QmlDesigner {

EventListDialog::EventListDialog(QWidget *parent)
    : QDialog(parent)
    , m_delegate(new EventListDelegate)
    , m_modifier(nullptr)
    , m_rewriter(nullptr)
    , m_table(new QTableView)
    , m_addAction(nullptr)
    , m_removeAction(nullptr)
    , m_textEdit(new QPlainTextEdit)
{
    setModal(true);
    setWindowFlag(Qt::Tool, true);

    m_modifier = new NotIndentingTextEditModifier(m_textEdit);
    m_textEdit->hide();

    m_table->installEventFilter(new TabWalker(this));
    m_table->setItemDelegate(m_delegate);
    m_table->setModel(new QSortFilterProxyModel);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->resizeColumnsToContents();

    auto *toolbar = new QToolBar;
    m_addAction = toolbar->addAction(Utils::Icons::PLUS_TOOLBAR.icon(), tr("Add Event"));
    m_removeAction = toolbar->addAction(Utils::Icons::MINUS.icon(), tr("Remove Selected Events"));

    auto *filterWidget = new FilterLineWidget;
    toolbar->addWidget(filterWidget);

    auto *tableLayout = new QVBoxLayout;
    tableLayout->setSpacing(0);
    tableLayout->addWidget(toolbar);
    tableLayout->addWidget(m_table);

    auto *box = new QHBoxLayout;
    box->addLayout(tableLayout);
    setLayout(box);

    connect(filterWidget, &FilterLineWidget::filterChanged, [this](const QString &str) {
        if (auto *fm = qobject_cast<QSortFilterProxyModel *>(m_table->model()))
            fm->setFilterFixedString(str);
    });
}

void EventListDialog::initialize(EventList &events)
{
    m_textEdit->setPlainText(events.read());

    if (!m_rewriter) {
        Model *model = events.model();
        m_modifier->setParent(model);

        m_rewriter = new RewriterView(QmlDesigner::RewriterView::Validate, model);
        m_rewriter->setTextModifier(m_modifier);
        m_rewriter->setCheckSemanticErrors(false);
        model->attachView(m_rewriter);

        if (auto *fm = qobject_cast<QSortFilterProxyModel *>(m_table->model())) {
            fm->setSourceModel(events.view()->eventListModel());
        }

        connect(m_addAction, &QAction::triggered, [this, &events]() {
            Event event;
            event.eventId = uniqueName(events.view()->eventListModel(), "event");
            events.view()->addEvent(event);
            events.write(m_textEdit->toPlainText());
        });

        connect(m_removeAction, &QAction::triggered, [this, &events]() {
            for (auto index : m_table->selectionModel()->selectedRows()) {
                QString eventId = index.data(Qt::DisplayRole).toString();
                events.view()->removeEvent(eventId);
            }
            events.write(m_textEdit->toPlainText());
        });

        connect(m_delegate,
                &EventListDelegate::eventIdChanged,
                [this, &events](const QString &oldId, const QString &newId) {
                    events.view()->renameEvent(oldId, newId);
                    events.write(m_textEdit->toPlainText());
                });

        connect(m_delegate,
                &EventListDelegate::shortcutChanged,
                [this, &events](const QString &id, const QString &text) {
                    events.view()->setShortcut(id, text);
                    events.write(m_textEdit->toPlainText());
                });

        connect(m_delegate,
                &EventListDelegate::descriptionChanged,
                [this, &events](const QString &id, const QString &text) {
                    events.view()->setDescription(id, text);
                    events.write(m_textEdit->toPlainText());
                });
    }

    m_table->setColumnHidden(EventListModel::connectColumn, true);
}

void EventListDialog::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    if (auto *view = EventList::nodeListView())
        view->reset();
}

} // namespace QmlDesigner.
