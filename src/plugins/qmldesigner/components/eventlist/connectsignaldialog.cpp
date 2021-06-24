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
#include "connectsignaldialog.h"
#include "eventlist.h"
#include "eventlistdelegate.h"
#include "eventlistview.h"
#include "filterlinewidget.h"
#include "nodelistview.h"
#include "eventlistutils.h"

#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QVBoxLayout>

namespace QmlDesigner {

QString eventListToSource(const QStringList &events)
{
    if (events.empty())
        return QString("{}");

    QString source("{\n");
    for (auto &&event : events)
        source += QString("EventSystem.triggerEvent(\"") + event + QString("\")\n");
    source += "}";
    return source;
}

QStringList eventListFromSource(const QString &source)
{
    QStringList out;
    for (auto &&substr : source.split("\n", Qt::SkipEmptyParts)) {
        auto trimmed = substr.trimmed();
        if (trimmed.startsWith("EventSystem.triggerEvent("))
            out << trimmed.section('\"', 1, 1);
    }
    return out;
}

ConnectSignalDialog::ConnectSignalDialog(QWidget *parent)
    : QDialog(parent)
    , m_table(new QTableView)
    , m_filter(new FilterLineWidget())
    , m_property()
{
    setWindowFlag(Qt::Tool, true);
    setModal(true);

    auto *filterModel = new QSortFilterProxyModel;
    auto *delegate = new EventListDelegate(m_table);

    m_table->installEventFilter(new TabWalker(this));
    m_table->setItemDelegate(delegate);
    m_table->setModel(filterModel);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->verticalHeader()->hide();
    polishPalette(m_table, QColor("#d87b00"));

    auto *box = new QVBoxLayout;
    box->addWidget(m_filter);
    box->addWidget(m_table);

    setLayout(box);

    connect(m_filter, &FilterLineWidget::filterChanged, [this](const QString &str) {
        if (auto *fm = qobject_cast<QSortFilterProxyModel *>(m_table->model()))
            fm->setFilterFixedString(str);
    });

    connect(delegate,
            &EventListDelegate::connectClicked,
            [this, filterModel](const QString &, bool) {
                if (m_property.isValid()) {
                    if (const auto *m = qobject_cast<const EventListModel *>(
                            filterModel->sourceModel())) {
                        QString source = eventListToSource(m->connectedEvents());
                        EventList::setSignalSource(m_property, source);
                    }
                }
            });
}

void ConnectSignalDialog::initialize(EventList &events, const SignalHandlerProperty &signal)
{
    m_filter->clear();

    auto *eventModel = events.view()->eventListModel();
    if (!eventModel)
        return;

    if (auto *fm = qobject_cast<QSortFilterProxyModel *>(m_table->model()))
        fm->setSourceModel(eventModel);

    m_property = signal;
    if (m_property.isValid()) {
        QString title = QString::fromUtf8(m_property.name());
        setWindowTitle(title);
        eventModel->connectEvents(eventListFromSource(m_property.source()));
    }

    if (QHeaderView *header = m_table->horizontalHeader()) {
        header->setSectionResizeMode(EventListModel::idColumn, QHeaderView::Stretch);
        header->setSectionResizeMode(EventListModel::descriptionColumn, QHeaderView::Stretch);
        header->setSectionResizeMode(EventListModel::shortcutColumn, QHeaderView::Stretch);
        header->resizeSection(EventListModel::connectColumn, 120);
        header->setStretchLastSection(false);
    }
}

QSize ConnectSignalDialog::sizeHint() const
{
    return QSize(522, 270);
}

} // namespace QmlDesigner.
