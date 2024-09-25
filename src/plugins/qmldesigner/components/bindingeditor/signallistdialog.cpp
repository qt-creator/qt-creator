// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

#include <memory>

namespace QmlDesigner {

std::unique_ptr<QWidget> createFilterWidget(Utils::FancyLineEdit *lineEdit)
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
    auto widget = std::make_unique<QWidget>();
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
    , m_signalListDelegate{Utils::makeUniqueObjectPtr<SignalListDelegate>()}
    , m_table(Utils::makeUniqueObjectPtr<QTableView>())
    , m_searchLine(Utils::makeUniqueObjectPtr<Utils::FancyLineEdit>())
{
    m_table->setItemDelegate(m_signalListDelegate.get());
    m_table->setFocusPolicy(Qt::NoFocus);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->verticalHeader()->hide();

    modifyPalette(m_table.get(), QColor("#d87b00"));

    auto *layout = new QVBoxLayout;
    auto filterWidget = createFilterWidget(m_searchLine.get());
    layout->addWidget(filterWidget.release());
    layout->addWidget(m_table.get());
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
    connect(m_searchLine.get(), &Utils::FancyLineEdit::filterChanged, eventFilterFun);
}

SignalListDelegate *SignalListDialog::signalListDelegate() const
{
    return m_signalListDelegate.get();
}

} // QmlDesigner namespace
