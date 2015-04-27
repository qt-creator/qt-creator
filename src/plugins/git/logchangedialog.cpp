/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    connect(this, SIGNAL(activated(QModelIndex)), this, SLOT(emitActivated(QModelIndex)));
}

bool LogChangeWidget::init(const QString &repository, const QString &commit, LogFlags flags)
{
    if (!populateLog(repository, commit, flags))
        return false;
    if (m_model->rowCount() > 0)
        return true;
    if (!(flags & Silent)) {
        VcsOutputWindow::appendError(
                    GitPlugin::instance()->client()->msgNoCommits(flags & IncludeRemotes));
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

void LogChangeWidget::emitActivated(const QModelIndex &index)
{
    if (index.isValid()) {
        QString commit = index.sibling(index.row(), Sha1Column).data().toString();
        if (!commit.isEmpty())
            emit activated(commit);
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
    GitClient *client = GitPlugin::instance()->client();
    QStringList arguments;
    arguments << QLatin1String("--max-count=1000") << QLatin1String("--format=%h:%s %d");
    arguments << (commit.isEmpty() ? QLatin1String("HEAD") : commit);
    if (!(flags & IncludeRemotes))
        arguments << QLatin1String("--not") << QLatin1String("--remotes");
    QString output;
    if (!client->synchronousLog(repository, arguments, &output, 0, VcsCommand::NoOutput))
        return false;
    foreach (const QString &line, output.split(QLatin1Char('\n'))) {
        const int colonPos = line.indexOf(QLatin1Char(':'));
        if (colonPos != -1) {
            QList<QStandardItem *> row;
            for (int c = 0; c < ColumnCount; ++c) {
                auto item = new QStandardItem;
                item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                if (line.endsWith(QLatin1Char(')'))) {
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
        m_resetTypeComboBox->addItem(tr("Hard"), QLatin1String("--hard"));
        m_resetTypeComboBox->addItem(tr("Mixed"), QLatin1String("--mixed"));
        m_resetTypeComboBox->addItem(tr("Soft"), QLatin1String("--soft"));
        GitClient *client = GitPlugin::instance()->client();
        m_resetTypeComboBox->setCurrentIndex(client->settings().intValue(
                                                 GitSettings::lastResetIndexKey));
        popUpLayout->addWidget(m_resetTypeComboBox);
        popUpLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Ignored));
    }

    popUpLayout->addWidget(m_dialogButtonBox);
    m_dialogButtonBox->addButton(QDialogButtonBox::Cancel);
    QPushButton *okButton = m_dialogButtonBox->addButton(QDialogButtonBox::Ok);
    layout->addLayout(popUpLayout);

    connect(m_dialogButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_dialogButtonBox, SIGNAL(rejected()), this, SLOT(reject()));

    connect(m_widget, SIGNAL(activated(QModelIndex)), okButton, SLOT(animateClick()));

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
            GitClient *client = GitPlugin::instance()->client();
            client->settings().setValue(GitSettings::lastResetIndexKey,
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

IconItemDelegate::IconItemDelegate(LogChangeWidget *widget, const QString &icon)
    : LogItemDelegate(widget)
    , m_icon(icon)
{
}

void IconItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QStyleOptionViewItem o = option;
    if (index.column() == 0 && hasIcon(index.row())) {
        const QSize size = option.decorationSize;
        painter->save();
        painter->drawPixmap(o.rect.x(), o.rect.y(), m_icon.pixmap(size.width(), size.height()));
        painter->restore();
        o.rect.translate(size.width(), 0);
    }
    QStyledItemDelegate::paint(painter, o, index);
}

} // namespace Internal
} // namespace Git
