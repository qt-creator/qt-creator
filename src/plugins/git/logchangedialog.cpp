/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "logchangedialog.h"
#include "gitplugin.h"
#include "gitclient.h"

#include <vcsbase/vcsoutputwindow.h>
#include <vcsbase/vcscommand.h>

#include <utils/qtcassert.h>

#include <QTreeView>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QDialogButtonBox>
#include <QItemSelectionModel>
#include <QVBoxLayout>
#include <QComboBox>
#include <QPainter>

using namespace VcsBase;

namespace Git {
namespace Internal {

enum Columns
{
    Sha1Column,
    SubjectColumn,
    ColumnCount
};

LogChangeWidget::LogChangeWidget(QWidget *parent)
    : Utils::TreeView(parent)
    , m_model(new QStandardItemModel(0, ColumnCount, this))
    , m_hasCustomDelegate(false)
{
    QStringList headers;
    headers << tr("Sha1")<< tr("Subject");
    m_model->setHorizontalHeaderLabels(headers);
    setModel(m_model);
    setMinimumWidth(300);
    setUniformRowHeights(true);
    setRootIsDecorated(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setActivationMode(Utils::DoubleClickActivation);
    connect(this, &LogChangeWidget::activated, this, &LogChangeWidget::emitCommitActivated);
}

bool LogChangeWidget::init(const QString &repository, const QString &commit, LogFlags flags)
{
    if (!populateLog(repository, commit, flags))
        return false;
    if (m_model->rowCount() > 0)
        return true;
    if (!(flags & Silent)) {
        VcsOutputWindow::appendError(
                    GitPlugin::client()->msgNoCommits(flags & IncludeRemotes));
    }
    return false;
}

QString LogChangeWidget::commit() const
{
    if (const QStandardItem *sha1Item = currentItem(Sha1Column))
        return sha1Item->text();
    return QString();
}

int LogChangeWidget::commitIndex() const
{
    const QModelIndex currentIndex = selectionModel()->currentIndex();
    if (currentIndex.isValid())
        return currentIndex.row();
    return -1;
}

QString LogChangeWidget::earliestCommit() const
{
    int rows = m_model->rowCount();
    if (rows) {
        if (const QStandardItem *item = m_model->item(rows - 1, Sha1Column))
            return item->text();
    }
    return QString();
}

void LogChangeWidget::setItemDelegate(QAbstractItemDelegate *delegate)
{
    Utils::TreeView::setItemDelegate(delegate);
    m_hasCustomDelegate = true;
}

void LogChangeWidget::emitCommitActivated(const QModelIndex &index)
{
    if (index.isValid()) {
        QString commit = index.sibling(index.row(), Sha1Column).data().toString();
        if (!commit.isEmpty())
            emit commitActivated(commit);
    }
}

void LogChangeWidget::selectionChanged(const QItemSelection &selected,
                                       const QItemSelection &deselected)
{
    Utils::TreeView::selectionChanged(selected, deselected);
    if (!m_hasCustomDelegate)
        return;
    const QModelIndexList previousIndexes = deselected.indexes();
    if (previousIndexes.isEmpty())
        return;
    const QModelIndex current = currentIndex();
    int row = current.row();
    int previousRow = previousIndexes.first().row();
    if (row < previousRow)
        qSwap(row, previousRow);
    for (int r = previousRow; r <= row; ++r) {
        update(current.sibling(r, 0));
        update(current.sibling(r, 1));
    }
}

bool LogChangeWidget::populateLog(const QString &repository, const QString &commit, LogFlags flags)
{
    const QString currentCommit = this->commit();
    int selected = currentCommit.isEmpty() ? 0 : -1;
    if (const int rowCount = m_model->rowCount())
        m_model->removeRows(0, rowCount);

    // Retrieve log using a custom format "Sha1:Subject [(refs)]"
    QStringList arguments;
    arguments << "--max-count=1000" << "--format=%h:%s %d";
    arguments << (commit.isEmpty() ? "HEAD" : commit);
    if (!(flags & IncludeRemotes))
        arguments << "--not" << "--remotes";
    arguments << "--";
    QString output;
    if (!GitPlugin::client()->synchronousLog(repository, arguments, &output, 0, VcsCommand::NoOutput))
        return false;
    const QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        const int colonPos = line.indexOf(':');
        if (colonPos != -1) {
            QList<QStandardItem *> row;
            for (int c = 0; c < ColumnCount; ++c) {
                auto item = new QStandardItem;
                item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                if (line.endsWith(')')) {
                    QFont font = item->font();
                    font.setBold(true);
                    item->setFont(font);
                }
                row.push_back(item);
            }
            const QString sha1 = line.left(colonPos);
            row[Sha1Column]->setText(sha1);
            row[SubjectColumn]->setText(line.right(line.size() - colonPos - 1));
            m_model->appendRow(row);
            if (selected == -1 && currentCommit == sha1)
                selected = m_model->rowCount() - 1;
        }
    }
    setCurrentIndex(m_model->index(selected, 0));
    return true;
}

const QStandardItem *LogChangeWidget::currentItem(int column) const
{
    const QModelIndex currentIndex = selectionModel()->currentIndex();
    if (currentIndex.isValid())
        return m_model->item(currentIndex.row(), column);
    return 0;
}

LogChangeDialog::LogChangeDialog(bool isReset, QWidget *parent) :
    QDialog(parent)
    , m_widget(new LogChangeWidget)
    , m_dialogButtonBox(new QDialogButtonBox(this))
    , m_resetTypeComboBox(0)
{
    auto layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(isReset ? tr("Reset to:") : tr("Select change:"), this));
    layout->addWidget(m_widget);
    auto popUpLayout = new QHBoxLayout;
    if (isReset) {
        popUpLayout->addWidget(new QLabel(tr("Reset type:"), this));
        m_resetTypeComboBox = new QComboBox(this);
        m_resetTypeComboBox->addItem(tr("Hard"), "--hard");
        m_resetTypeComboBox->addItem(tr("Mixed"), "--mixed");
        m_resetTypeComboBox->addItem(tr("Soft"), "--soft");
        m_resetTypeComboBox->setCurrentIndex(GitPlugin::client()->settings().intValue(
                                                 GitSettings::lastResetIndexKey));
        popUpLayout->addWidget(m_resetTypeComboBox);
        popUpLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));
    }

    popUpLayout->addWidget(m_dialogButtonBox);
    m_dialogButtonBox->addButton(QDialogButtonBox::Cancel);
    QPushButton *okButton = m_dialogButtonBox->addButton(QDialogButtonBox::Ok);
    layout->addLayout(popUpLayout);

    connect(m_dialogButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_dialogButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    connect(m_widget, &LogChangeWidget::activated, okButton, [okButton] { okButton->animateClick(); });

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    resize(600, 400);
}

bool LogChangeDialog::runDialog(const QString &repository,
                                const QString &commit,
                                LogChangeWidget::LogFlags flags)
{
    if (!m_widget->init(repository, commit, flags))
        return false;

    if (QDialog::exec() == QDialog::Accepted) {
        if (m_resetTypeComboBox) {
            GitPlugin::client()->settings().setValue(GitSettings::lastResetIndexKey,
                                                     m_resetTypeComboBox->currentIndex());
        }
        return true;
    }
    return false;
}

QString LogChangeDialog::commit() const
{
    return m_widget->commit();
}

int LogChangeDialog::commitIndex() const
{
    return m_widget->commitIndex();
}

QString LogChangeDialog::resetFlag() const
{
    if (!m_resetTypeComboBox)
        return QString();
    return m_resetTypeComboBox->itemData(m_resetTypeComboBox->currentIndex()).toString();
}

LogChangeWidget *LogChangeDialog::widget() const
{
    return m_widget;
}

LogItemDelegate::LogItemDelegate(LogChangeWidget *widget) : m_widget(widget)
{
    m_widget->setItemDelegate(this);
}

int LogItemDelegate::currentRow() const
{
    return m_widget->commitIndex();
}

IconItemDelegate::IconItemDelegate(LogChangeWidget *widget, const Utils::Icon &icon)
    : LogItemDelegate(widget)
    , m_icon(icon.icon())
{
}

void IconItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QStyleOptionViewItem o = option;
    if (index.column() == 0 && hasIcon(index.row())) {
        const QSize size = option.decorationSize;
        painter->drawPixmap(o.rect.x(), o.rect.y(), m_icon.pixmap(size.width(), size.height()));
        o.rect.setLeft(size.width());
    }
    QStyledItemDelegate::paint(painter, o, index);
}

} // namespace Internal
} // namespace Git
