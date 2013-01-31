/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gerritdialog.h"
#include "gerritmodel.h"
#include "gerritparameters.h"

#include <utils/filterlineedit.h>
#include <coreplugin/icore.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QSpacerItem>
#include <QLabel>
#include <QTextBrowser>
#include <QTreeView>
#include <QHeaderView>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QSortFilterProxyModel>
#include <QGroupBox>
#include <QUrl>
#include <QClipboard>
#include <QApplication>
#include <QProcess>
#include <QStringListModel>
#include <QCompleter>

namespace Gerrit {
namespace Internal {

static const int layoutSpacing  = 5;
static const int maxTitleWidth = 350;

QueryValidatingLineEdit::QueryValidatingLineEdit(QWidget *parent)
    : Utils::FilterLineEdit(parent)
    , m_valid(true)
    , m_okTextColor(palette().color(QPalette::Active, QPalette::Text))
    , m_errorTextColor(Qt::red)
{
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(setValid()));
}

void QueryValidatingLineEdit::setTextColor(const QColor &c)
{
    QPalette pal = palette();
    pal.setColor(QPalette::Active, QPalette::Text, c);
    setPalette(pal);
}

void QueryValidatingLineEdit::setValid()
{
    if (!m_valid) {
        m_valid = true;
        setTextColor(m_okTextColor);
    }
}

void QueryValidatingLineEdit::setInvalid()
{
    if (m_valid) {
        m_valid = false;
        setTextColor(m_errorTextColor);
    }
}

GerritDialog::GerritDialog(const QSharedPointer<GerritParameters> &p,
                           QWidget *parent)
    : QDialog(parent)
    , m_parameters(p)
    , m_filterModel(new QSortFilterProxyModel(this))
    , m_model(new GerritModel(p, this))
    , m_queryModel(new QStringListModel(this))
    , m_treeView(new QTreeView)
    , m_detailsBrowser(new QTextBrowser)
    , m_queryLineEdit(new QueryValidatingLineEdit)
    , m_filterLineEdit(new Utils::FilterLineEdit)
    , m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Close))
    , m_fetchRunning(false)
{
    setWindowTitle(tr("Gerrit %1@%2").arg(p->user, p->host));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QGroupBox *changesGroup = new QGroupBox(tr("Changes"));
    QVBoxLayout *changesLayout = new QVBoxLayout(changesGroup);
    changesLayout->setMargin(layoutSpacing);
    QHBoxLayout *filterLayout = new QHBoxLayout;
    QLabel *queryLabel = new QLabel(tr("&Query:"));
    queryLabel->setBuddy(m_queryLineEdit);
    m_queryLineEdit->setFixedWidth(400);
    m_queryLineEdit->setPlaceholderText(tr("Change #, SHA-1, tr:id, owner:email or reviewer:email"));
    m_queryModel->setStringList(m_parameters->savedQueries);
    QCompleter *completer = new QCompleter(this);
    completer->setModel(m_queryModel);
    m_queryLineEdit->setSpecialCompleter(completer);
    filterLayout->addWidget(queryLabel);
    filterLayout->addWidget(m_queryLineEdit);
    filterLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
    m_filterLineEdit->setFixedWidth(300);
    filterLayout->addWidget(m_filterLineEdit);
    connect(m_filterLineEdit, SIGNAL(filterChanged(QString)),
            m_filterModel, SLOT(setFilterFixedString(QString)));
    connect(m_queryLineEdit, SIGNAL(returnPressed()),
            this, SLOT(slotRefresh()));
    connect(m_model, SIGNAL(queryError()), m_queryLineEdit, SLOT(setInvalid()));
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    changesLayout->addLayout(filterLayout);
    changesLayout->addWidget(m_treeView);

    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterRole(GerritModel::FilterRole);
    m_treeView->setModel(m_filterModel);
    m_treeView->setMinimumWidth(600);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setSortingEnabled(true);

    QItemSelectionModel *selectionModel = m_treeView->selectionModel();
    connect(selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(slotCurrentChanged()));
    connect(m_treeView, SIGNAL(doubleClicked(QModelIndex)),
            this, SLOT(slotDoubleClicked(QModelIndex)));

    QGroupBox *detailsGroup = new QGroupBox(tr("Details"));
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsGroup);
    detailsLayout->setMargin(layoutSpacing);
    m_detailsBrowser->setOpenExternalLinks(true);
    m_detailsBrowser->setTextInteractionFlags(Qt::TextBrowserInteraction);
    detailsLayout->addWidget(m_detailsBrowser);

    m_displayButton = addActionButton(tr("Diff..."), SLOT(slotFetchDisplay()));
    m_applyButton = addActionButton(tr("Apply..."), SLOT(slotFetchApply()));
    m_checkoutButton = addActionButton(tr("Checkout..."), SLOT(slotFetchCheckout()));
    m_refreshButton = addActionButton(tr("Refresh"), SLOT(slotRefresh()));

    connect(m_model, SIGNAL(refreshStateChanged(bool)),
            m_refreshButton, SLOT(setDisabled(bool)));
    connect(m_model, SIGNAL(refreshStateChanged(bool)),
            this, SLOT(slotRefreshStateChanged(bool)));
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QSplitter *splitter = new QSplitter(Qt::Vertical, this);
    splitter->addWidget(changesGroup);
    splitter->addWidget(detailsGroup);
    splitter->setSizes(QList<int>() <<  400 << 200);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(splitter);
    mainLayout->addWidget(m_buttonBox);

    slotCurrentChanged();
    slotRefresh();

    resize(QSize(950, 600));
    m_treeView->setFocus();
}

QPushButton *GerritDialog::addActionButton(const QString &text, const char *buttonSlot)
{
    QPushButton *button = m_buttonBox->addButton(text, QDialogButtonBox::ActionRole);
    connect(button, SIGNAL(clicked()), this, buttonSlot);
    return button;
}

void GerritDialog::updateCompletions(const QString &query)
{
    if (query.isEmpty())
        return;
    QStringList &queries = m_parameters->savedQueries;
    queries.removeAll(query);
    queries.prepend(query);
    m_queryModel->setStringList(queries);
    m_parameters->saveQueries(Core::ICore::instance()->settings());
}

GerritDialog::~GerritDialog()
{
}

void GerritDialog::slotDoubleClicked(const QModelIndex &i)
{
    if (const QStandardItem *item = itemAt(i))
        QDesktopServices::openUrl(QUrl(m_model->change(item->row())->url));
}

void GerritDialog::slotRefreshStateChanged(bool v)
{
    if (!v && m_model->rowCount()) {
        for (int c = 0; c < GerritModel::ColumnCount; ++c)
            m_treeView->resizeColumnToContents(c);
        if (m_treeView->columnWidth(GerritModel::TitleColumn) > maxTitleWidth)
            m_treeView->setColumnWidth(GerritModel::TitleColumn, maxTitleWidth);
    }
}

void GerritDialog::slotFetchDisplay()
{
    if (const QStandardItem *item = currentItem())
        emit fetchDisplay(m_model->change(item->row()));
}

void GerritDialog::slotFetchApply()
{
    if (const QStandardItem *item = currentItem())
        emit fetchApply(m_model->change(item->row()));
}

void GerritDialog::slotFetchCheckout()
{
    if (const QStandardItem *item = currentItem())
        emit fetchCheckout(m_model->change(item->row()));
}

void GerritDialog::slotRefresh()
{
    const QString &query = m_queryLineEdit->text().trimmed();
    updateCompletions(query);
    m_model->refresh(query);
    m_treeView->sortByColumn(-1);
}

const QStandardItem *GerritDialog::itemAt(const QModelIndex &i, int column) const
{
    if (i.isValid()) {
        const QModelIndex source = m_filterModel->mapToSource(i);
        if (source.isValid())
            return m_model->item(source.row(), column);
    }
    return 0;
}

const QStandardItem *GerritDialog::currentItem(int column) const
{
    const QModelIndex index = m_treeView->selectionModel()->currentIndex();
    if (index.isValid())
        return itemAt(index, column);
    return 0;
}

void GerritDialog::updateButtons()
{
    const bool enabled = !m_fetchRunning && m_treeView->selectionModel()->currentIndex().isValid();
    m_displayButton->setEnabled(enabled);
    m_applyButton->setEnabled(enabled);
    m_checkoutButton->setEnabled(enabled);
}

void GerritDialog::slotCurrentChanged()
{
    const QModelIndex current = m_treeView->selectionModel()->currentIndex();
    const bool valid = current.isValid();
    if (valid) {
        const int row = m_filterModel->mapToSource(current).row();
        m_detailsBrowser->setText(m_model->change(row)->toHtml());
    } else {
        m_detailsBrowser->setText(QString());
    }
    updateButtons();
}

void GerritDialog::fetchStarted(const QSharedPointer<Gerrit::Internal::GerritChange> &change)
{
    // Disable buttons to prevent parallel gerrit operations which can cause mix-ups.
    m_fetchRunning = true;
    updateButtons();
    const QString toolTip = tr("Fetching \"%1\"...").arg(change->title);
    m_displayButton->setToolTip(toolTip);
    m_applyButton->setToolTip(toolTip);
    m_checkoutButton->setToolTip(toolTip);
}

void GerritDialog::fetchFinished()
{
    m_fetchRunning = false;
    updateButtons();
    m_displayButton->setToolTip(QString());
    m_applyButton->setToolTip(QString());
    m_checkoutButton->setToolTip(QString());
}

} // namespace Internal
} // namespace Gerrit
