// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "topicchooser.h"

#include "helptr.h"

#include <utils/fancylineedit.h>
#include <utils/layoutbuilder.h>
#include <utils/utilstr.h>

#include <coreplugin/icore.h>

#include <QMap>
#include <QUrl>

#include <QKeyEvent>
#include <QDialogButtonBox>
#include <QListView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>

const int kInitialWidth = 400;
const int kInitialHeight = 220;
const char kPreferenceDialogSize[] = "Core/TopicChooserSize";

TopicChooser::TopicChooser(QWidget *parent, const QString &keyword,
        const QMultiMap<QString, QUrl> &links)
    : QDialog(parent)
    , m_filterModel(new QSortFilterProxyModel(this))
{
    const QSize initialSize(kInitialWidth, kInitialHeight);
    resize(Core::ICore::settings()->value(kPreferenceDialogSize, initialSize).toSize());

    setWindowTitle(::Help::Tr::tr("Choose Topic"));

    QStandardItemModel *model = new QStandardItemModel(this);
    m_filterModel->setSourceModel(model);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    QMultiMap<QString, QUrl>::const_iterator it = links.constBegin();
    for (; it != links.constEnd(); ++it) {
        m_links.append(it.value());
        QStandardItem *item = new QStandardItem(it.key());
        item->setToolTip(it.value().toString());
        model->appendRow(item);
    }

    m_lineEdit = new Utils::FancyLineEdit;
    m_lineEdit->setFiltering(true);
    m_lineEdit->installEventFilter(this);
    setFocusProxy(m_lineEdit);

    m_listWidget = new QListView;
    m_listWidget->setModel(m_filterModel);
    m_listWidget->setUniformItemSizes(true);
    m_listWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    if (m_filterModel->rowCount() != 0)
        m_listWidget->setCurrentIndex(m_filterModel->index(0, 0));

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    using namespace Layouting;
    Column {
        ::Help::Tr::tr("Choose a topic for <b>%1</b>:").arg(keyword),
        m_lineEdit,
        m_listWidget,
        buttonBox,
    }.attachTo(this);

    connect(buttonBox, &QDialogButtonBox::accepted,
            this, &TopicChooser::acceptDialog);
    connect(buttonBox, &QDialogButtonBox::rejected,
            this, &TopicChooser::reject);
    connect(m_listWidget, &QListView::activated,
            this, &TopicChooser::activated);
    connect(m_lineEdit, &Utils::FancyLineEdit::filterChanged,
            this, &TopicChooser::setFilter);
}

TopicChooser::~TopicChooser()
{
    Core::ICore::settings()->setValueWithDefault(kPreferenceDialogSize,
                                           size(),
                                           QSize(kInitialWidth, kInitialHeight));
}

QUrl TopicChooser::link() const
{
    if (m_activedIndex.isValid())
        return m_links.at(m_filterModel->mapToSource(m_activedIndex).row());
    return QUrl();
}

void TopicChooser::acceptDialog()
{
    m_activedIndex = m_listWidget->currentIndex();
    accept();
}

void TopicChooser::setFilter(const QString &pattern)
{
    m_filterModel->setFilterFixedString(pattern);
    if (m_filterModel->rowCount() != 0 && !m_listWidget->currentIndex().isValid())
        m_listWidget->setCurrentIndex(m_filterModel->index(0, 0));
}

void TopicChooser::activated(const QModelIndex &index)
{
    m_activedIndex = index;
    accept();
}

bool TopicChooser::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_lineEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent*>(event);
        int dIndex = 0;
        switch (ke->key()) {
        case Qt::Key_Up:
            dIndex = -1;
            break;
        case Qt::Key_Down:
            dIndex = +1;
            break;
        case Qt::Key_PageUp:
            dIndex = -5;
            break;
        case Qt::Key_PageDown:
            dIndex = +5;
            break;
        default:
            break;
        }
        if (dIndex != 0) {
            QModelIndex idx = m_listWidget->currentIndex();
            int newIndex = qMin(m_filterModel->rowCount(idx.parent()) - 1,
                                qMax(0, idx.row() + dIndex));
            idx = m_filterModel->index(newIndex, idx.column(), idx.parent());
            if (idx.isValid())
                m_listWidget->setCurrentIndex(idx);
            return true;
        }
    } else if (m_lineEdit && event->type() == QEvent::FocusIn
        && static_cast<QFocusEvent *>(event)->reason() != Qt::MouseFocusReason) {
        m_lineEdit->selectAll();
        m_lineEdit->setFocus();
    }
    return QDialog::eventFilter(object, event);
}
