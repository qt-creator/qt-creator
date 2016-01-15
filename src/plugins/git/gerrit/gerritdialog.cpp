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

#include "gerritdialog.h"
#include "gerritmodel.h"
#include "gerritparameters.h"

#include <utils/qtcassert.h>
#include <utils/fancylineedit.h>
#include <utils/itemviews.h>
#include <utils/progressindicator.h>
#include <utils/theme/theme.h>
#include <coreplugin/icore.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QSpacerItem>
#include <QLabel>
#include <QTextBrowser>
#include <QTreeView>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDesktopServices>
#include <QSortFilterProxyModel>
#include <QGroupBox>
#include <QUrl>
#include <QStringListModel>
#include <QCompleter>

namespace Gerrit {
namespace Internal {

static const int layoutSpacing  = 5;
static const int maxTitleWidth = 350;

GerritDialog::GerritDialog(const QSharedPointer<GerritParameters> &p,
                           QWidget *parent)
    : QDialog(parent)
    , m_parameters(p)
    , m_filterModel(new QSortFilterProxyModel(this))
    , m_model(new GerritModel(p, this))
    , m_queryModel(new QStringListModel(this))
    , m_treeView(new Utils::TreeView)
    , m_detailsBrowser(new QTextBrowser)
    , m_queryLineEdit(new Utils::FancyLineEdit)
    , m_filterLineEdit(new Utils::FancyLineEdit)
    , m_repositoryChooser(new Utils::PathChooser)
    , m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Close))
    , m_repositoryChooserLabel(new QLabel(tr("Apply in:") + QLatin1Char(' '), this))
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
    m_queryLineEdit->setOkColor(Utils::creatorTheme()->color(Utils::Theme::TextColorNormal));
    m_queryLineEdit->setErrorColor(Utils::creatorTheme()->color(Utils::Theme::TextColorError));
    m_queryLineEdit->setValidationFunction([this](Utils::FancyLineEdit *, QString *) {
                                               return m_model->state() != GerritModel::Error;
                                           });
    filterLayout->addWidget(queryLabel);
    filterLayout->addWidget(m_queryLineEdit);
    filterLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Ignored));
    m_filterLineEdit->setFixedWidth(300);
    m_filterLineEdit->setFiltering(true);
    filterLayout->addWidget(m_filterLineEdit);
    connect(m_filterLineEdit, &Utils::FancyLineEdit::filterChanged,
            m_filterModel, &QSortFilterProxyModel::setFilterFixedString);
    connect(m_queryLineEdit, &QLineEdit::returnPressed, this, &GerritDialog::slotRefresh);
    connect(m_model, &GerritModel::stateChanged, m_queryLineEdit, &Utils::FancyLineEdit::validate);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    changesLayout->addLayout(filterLayout);
    changesLayout->addWidget(m_treeView);

    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterRole(GerritModel::FilterRole);
    m_filterModel->setSortRole(GerritModel::SortRole);
    m_treeView->setRootIsDecorated(true);
    m_treeView->setModel(m_filterModel);
    m_treeView->setMinimumWidth(600);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setSortingEnabled(true);
    m_treeView->setActivationMode(Utils::DoubleClickActivation);

    connect(&m_progressIndicatorTimer, &QTimer::timeout,
            [this]() { setProgressIndicatorVisible(true); });
    m_progressIndicatorTimer.setSingleShot(true);
    m_progressIndicatorTimer.setInterval(50); // don't show progress for < 50ms tasks

    m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicator::Large,
                                                       m_treeView);
    m_progressIndicator->attachToWidget(m_treeView->viewport());
    m_progressIndicator->hide();

    connect(m_model, &GerritModel::stateChanged, this, &GerritDialog::manageProgressIndicator);

    QItemSelectionModel *selectionModel = m_treeView->selectionModel();
    connect(selectionModel, &QItemSelectionModel::currentChanged,
            this, &GerritDialog::slotCurrentChanged);
    connect(m_treeView, &QAbstractItemView::activated,
            this, &GerritDialog::slotActivated);

    QGroupBox *detailsGroup = new QGroupBox(tr("Details"));
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsGroup);
    detailsLayout->setMargin(layoutSpacing);
    m_detailsBrowser->setOpenExternalLinks(true);
    m_detailsBrowser->setTextInteractionFlags(Qt::TextBrowserInteraction);
    detailsLayout->addWidget(m_detailsBrowser);

    m_repositoryChooser->setExpectedKind(Utils::PathChooser::Directory);
    m_repositoryChooser->setHistoryCompleter(QLatin1String("Git.RepoDir.History"));
    QHBoxLayout *repoPathLayout = new QHBoxLayout;
    repoPathLayout->addWidget(m_repositoryChooserLabel);
    repoPathLayout->addWidget(m_repositoryChooser);
    detailsLayout->addLayout(repoPathLayout);

    m_displayButton = addActionButton(tr("&Show"), [this]() { slotFetchDisplay(); });
    m_cherryPickButton = addActionButton(tr("Cherry &Pick"), [this]() { slotFetchCherryPick(); });
    m_checkoutButton = addActionButton(tr("C&heckout"), [this]() { slotFetchCheckout(); });
    m_refreshButton = addActionButton(tr("&Refresh"), [this]() { slotRefresh(); });

    connect(m_model, &GerritModel::refreshStateChanged,
            m_refreshButton, &QWidget::setDisabled);
    connect(m_model, &GerritModel::refreshStateChanged,
            this, &GerritDialog::slotRefreshStateChanged);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

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
    m_refreshButton->setDefault(true);
}

QString GerritDialog::repositoryPath() const
{
    return m_repositoryChooser->path();
}

void GerritDialog::setCurrentPath(const QString &path)
{
    m_repositoryChooser->setPath(path);
}

QPushButton *GerritDialog::addActionButton(const QString &text,
                                           const std::function<void()> &buttonSlot)
{
    QPushButton *button = m_buttonBox->addButton(text, QDialogButtonBox::ActionRole);
    connect(button, &QPushButton::clicked, this, buttonSlot);
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
    m_parameters->saveQueries(Core::ICore::settings());
}

GerritDialog::~GerritDialog()
{
}

void GerritDialog::slotActivated(const QModelIndex &i)
{
    const QModelIndex source = m_filterModel->mapToSource(i);
    if (source.isValid())
        QDesktopServices::openUrl(QUrl(m_model->change(source)->url));
}

void GerritDialog::slotRefreshStateChanged(bool v)
{
    if (!v && m_model->rowCount()) {
        m_treeView->expandAll();
        for (int c = 0; c < GerritModel::ColumnCount; ++c)
            m_treeView->resizeColumnToContents(c);
        if (m_treeView->columnWidth(GerritModel::TitleColumn) > maxTitleWidth)
            m_treeView->setColumnWidth(GerritModel::TitleColumn, maxTitleWidth);
    }
}

void GerritDialog::slotFetchDisplay()
{
    const QModelIndex index = currentIndex();
    if (index.isValid())
        emit fetchDisplay(m_model->change(index));
}

void GerritDialog::slotFetchCherryPick()
{
    const QModelIndex index = currentIndex();
    if (index.isValid())
        emit fetchCherryPick(m_model->change(index));
}

void GerritDialog::slotFetchCheckout()
{
    const QModelIndex index = currentIndex();
    if (index.isValid())
        emit fetchCheckout(m_model->change(index));
}

void GerritDialog::slotRefresh()
{
    const QString &query = m_queryLineEdit->text().trimmed();
    updateCompletions(query);
    m_model->refresh(query);
    m_treeView->sortByColumn(-1);
}

void GerritDialog::manageProgressIndicator()
{
    if (m_model->state() == GerritModel::Running) {
        m_progressIndicatorTimer.start();
    } else {
        m_progressIndicatorTimer.stop();
        setProgressIndicatorVisible(false);
    }
}

QModelIndex GerritDialog::currentIndex() const
{
    const QModelIndex index = m_treeView->selectionModel()->currentIndex();
    return index.isValid() ? m_filterModel->mapToSource(index) : QModelIndex();
}

void GerritDialog::updateButtons()
{
    const bool enabled = !m_fetchRunning && m_treeView->selectionModel()->currentIndex().isValid();
    m_displayButton->setEnabled(enabled);
    m_cherryPickButton->setEnabled(enabled);
    m_checkoutButton->setEnabled(enabled);
}

void GerritDialog::slotCurrentChanged()
{
    const QModelIndex current = currentIndex();
    m_detailsBrowser->setText(current.isValid() ? m_model->toHtml(current) : QString());
    updateButtons();
}

void GerritDialog::fetchStarted(const QSharedPointer<GerritChange> &change)
{
    // Disable buttons to prevent parallel gerrit operations which can cause mix-ups.
    m_fetchRunning = true;
    updateButtons();
    const QString toolTip = tr("Fetching \"%1\"...").arg(change->title);
    m_displayButton->setToolTip(toolTip);
    m_cherryPickButton->setToolTip(toolTip);
    m_checkoutButton->setToolTip(toolTip);
}

void GerritDialog::fetchFinished()
{
    m_fetchRunning = false;
    updateButtons();
    m_displayButton->setToolTip(QString());
    m_cherryPickButton->setToolTip(QString());
    m_checkoutButton->setToolTip(QString());
}

void GerritDialog::setProgressIndicatorVisible(bool v)
{
    m_progressIndicator->setVisible(v);
}

} // namespace Internal
} // namespace Gerrit
