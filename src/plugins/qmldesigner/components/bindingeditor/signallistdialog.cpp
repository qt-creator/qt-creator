/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "signallistdialog.h"

#include "signallist.h"
#include "signallistdelegate.h"

#include <qmldesignerplugin.h>

#include <theme.h>
#include <utils/fancylineedit.h>
#include <utils/stylehelper.h>
#include <utils/utilsicons.h>

#include <QStandardItemModel>
#include <QPainter>
#include <QTableView>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QSortFilterProxyModel>

namespace QmlDesigner {

QWidget *createFilterWidget(Utils::FancyLineEdit *lineEdit)
{
    const QString unicode = Theme::getIconUnicode(Theme::Icon::search);
    const QString fontName = "qtds_propertyIconFont.ttf";
    QIcon icon = Utils::StyleHelper::getIconFromIconFont(fontName, unicode, 28, 28);
    auto *label = new QLabel;
    label->setPixmap(icon.pixmap(QSize(24, 24)));
    label->setAlignment(Qt::AlignCenter);
    lineEdit->setPlaceholderText(QObject::tr("<Filter>", "Library search input hint text"));
    lineEdit->setDragEnabled(false);
    lineEdit->setMinimumWidth(75);
    lineEdit->setTextMargins(0, 0, 20, 0);
    lineEdit->setFiltering(true);
    auto *box = new QHBoxLayout;
    box->addWidget(label);
    box->addWidget(lineEdit);
    auto *widget = new QWidget;
    widget->setLayout(box);
    return widget;
}

void modifyPalette(QTableView *view, const QColor &selectionColor)
{
    QPalette p = view->palette();
    p.setColor(QPalette::AlternateBase, p.color(QPalette::Base).lighter(120));
    p.setColor(QPalette::Highlight, selectionColor);
    view->setPalette(p);
    view->setAlternatingRowColors(true);
}


SignalListDialog::SignalListDialog(QWidget *parent)
    : QDialog(parent)
    , m_table(new QTableView())
    , m_searchLine(new Utils::FancyLineEdit())
{
    auto *signalListDelegate = new SignalListDelegate(m_table);
    m_table->setItemDelegate(signalListDelegate);
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->verticalHeader()->hide();

    modifyPalette(m_table, QColor("#d87b00"));

    auto *layout = new QVBoxLayout;
    layout->addWidget(createFilterWidget(m_searchLine));
    layout->addWidget(m_table);
    setLayout(layout);

    setWindowFlag(Qt::Tool, true);
    setModal(true);
    resize(600, 480);
}

SignalListDialog::~SignalListDialog()
{
}

void SignalListDialog::initialize(QStandardItemModel *model)
{
    m_searchLine->clear();

    auto *proxyModel = new SignalListFilterModel(this);
    proxyModel->setSourceModel(model);
    m_table->setModel(proxyModel);

    QHeaderView *header = m_table->horizontalHeader();
    header->setSectionResizeMode(SignalListModel::TargetColumn, QHeaderView::Stretch);
    header->setSectionResizeMode(SignalListModel::SignalColumn, QHeaderView::Stretch);
    header->resizeSection(SignalListModel::ButtonColumn, 120);
    header->setStretchLastSection(false);

    auto eventFilterFun = [this](const QString &str) {
        if (auto *fm = qobject_cast<SignalListFilterModel *>(m_table->model())) {
            const QRegularExpression::PatternOption option
                = fm->filterCaseSensitivity() == Qt::CaseInsensitive
                      ? QRegularExpression::CaseInsensitiveOption
                      : QRegularExpression::NoPatternOption;
            fm->setFilterRegularExpression(
                QRegularExpression(QRegularExpression::escape(str), option));
        }
    };
    connect(m_searchLine, &Utils::FancyLineEdit::filterChanged, eventFilterFun);
}

QTableView *SignalListDialog::tableView() const
{
    return m_table;
}

} // QmlDesigner namespace
