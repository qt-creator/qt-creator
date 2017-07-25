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
#include "ui_gerritdialog.h"
#include "gerritmodel.h"
#include "gerritparameters.h"

#include "../gitplugin.h"
#include "../gitclient.h"

#include <coreplugin/icore.h>

#include <utils/asconst.h>
#include <utils/hostosinfo.h>
#include <utils/progressindicator.h>
#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QCompleter>
#include <QDesktopServices>
#include <QFileInfo>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QUrl>

namespace Gerrit {
namespace Internal {

static const int maxTitleWidth = 350;

GerritDialog::GerritDialog(const QSharedPointer<GerritParameters> &p,
                           const QSharedPointer<GerritServer> &s,
                           const QString &repository,
                           QWidget *parent)
    : QDialog(parent)
    , m_parameters(p)
    , m_server(s)
    , m_filterModel(new QSortFilterProxyModel(this))
    , m_ui(new Ui::GerritDialog)
    , m_model(new GerritModel(p, this))
    , m_queryModel(new QStringListModel(this))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    m_ui->setupUi(this);
    m_ui->remoteComboBox->setParameters(m_parameters);
    m_ui->remoteComboBox->setFallbackEnabled(true);
    m_queryModel->setStringList(m_parameters->savedQueries);
    QCompleter *completer = new QCompleter(this);
    completer->setModel(m_queryModel);
    m_ui->queryLineEdit->setSpecialCompleter(completer);
    m_ui->queryLineEdit->setOkColor(Utils::creatorTheme()->color(Utils::Theme::TextColorNormal));
    m_ui->queryLineEdit->setErrorColor(Utils::creatorTheme()->color(Utils::Theme::TextColorError));
    m_ui->queryLineEdit->setValidationFunction([this](Utils::FancyLineEdit *, QString *) {
                                               return m_model->state() != GerritModel::Error;
                                           });
    m_ui->filterLineEdit->setFiltering(true);
    connect(m_ui->filterLineEdit, &Utils::FancyLineEdit::filterChanged,
            m_filterModel, &QSortFilterProxyModel::setFilterFixedString);
    connect(m_ui->queryLineEdit, &QLineEdit::returnPressed, this, &GerritDialog::refresh);
    connect(m_model, &GerritModel::stateChanged, m_ui->queryLineEdit, &Utils::FancyLineEdit::validate);
    connect(m_ui->remoteComboBox, &GerritRemoteChooser::remoteChanged,
            this, &GerritDialog::remoteChanged);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterRole(GerritModel::FilterRole);
    m_filterModel->setSortRole(GerritModel::SortRole);
    m_ui->treeView->setModel(m_filterModel);
    m_ui->treeView->setActivationMode(Utils::DoubleClickActivation);

    connect(&m_progressIndicatorTimer, &QTimer::timeout,
            [this]() { setProgressIndicatorVisible(true); });
    m_progressIndicatorTimer.setSingleShot(true);
    m_progressIndicatorTimer.setInterval(50); // don't show progress for < 50ms tasks

    m_progressIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicatorSize::Large,
                                                       m_ui->treeView);
    m_progressIndicator->attachToWidget(m_ui->treeView->viewport());
    m_progressIndicator->hide();

    connect(m_model, &GerritModel::stateChanged, this, &GerritDialog::manageProgressIndicator);

    QItemSelectionModel *selectionModel = m_ui->treeView->selectionModel();
    connect(selectionModel, &QItemSelectionModel::currentChanged,
            this, &GerritDialog::slotCurrentChanged);
    connect(m_ui->treeView, &QAbstractItemView::activated,
            this, &GerritDialog::slotActivated);

    m_displayButton = addActionButton(tr("&Show"), [this]() { slotFetchDisplay(); });
    m_cherryPickButton = addActionButton(tr("Cherry &Pick"), [this]() { slotFetchCherryPick(); });
    m_checkoutButton = addActionButton(tr("C&heckout"), [this]() { slotFetchCheckout(); });
    m_refreshButton = addActionButton(tr("&Refresh"), [this]() { refresh(); });

    connect(m_model, &GerritModel::refreshStateChanged,
            m_refreshButton, &QWidget::setDisabled);
    connect(m_model, &GerritModel::refreshStateChanged,
            this, &GerritDialog::slotRefreshStateChanged);
    connect(m_model, &GerritModel::errorText,
            this, [this](const QString &text) {
        if (text.contains("returned error: 401"))
            updateRemotes(true);
    }, Qt::QueuedConnection);

    setCurrentPath(repository);
    slotCurrentChanged();

    m_ui->treeView->setFocus();
    m_refreshButton->setDefault(true);
}

QString GerritDialog::repositoryPath() const
{
    return m_repository;
}

void GerritDialog::setCurrentPath(const QString &path)
{
    if (path == m_repository)
        return;
    m_repository = path;
    m_ui->repositoryLabel->setText(Git::Internal::GitPlugin::msgRepositoryLabel(path));
    updateRemotes();
}

QPushButton *GerritDialog::addActionButton(const QString &text,
                                           const std::function<void()> &buttonSlot)
{
    QPushButton *button = m_ui->buttonBox->addButton(text, QDialogButtonBox::ActionRole);
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
    delete m_ui;
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
        m_ui->treeView->expandAll();
        for (int c = 0; c < GerritModel::ColumnCount; ++c)
            m_ui->treeView->resizeColumnToContents(c);
        if (m_ui->treeView->columnWidth(GerritModel::TitleColumn) > maxTitleWidth)
            m_ui->treeView->setColumnWidth(GerritModel::TitleColumn, maxTitleWidth);
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

void GerritDialog::refresh()
{
    const QString &query = m_ui->queryLineEdit->text().trimmed();
    updateCompletions(query);
    m_model->refresh(m_server, query);
    m_ui->treeView->sortByColumn(-1);
}

void GerritDialog::remoteChanged()
{
    const GerritServer server = m_ui->remoteComboBox->currentServer();
    if (QSharedPointer<GerritServer> modelServer = m_model->server()) {
        if (*modelServer == server)
           return;
    }
    *m_server = server;
    refresh();
}

void GerritDialog::updateRemotes(bool forceReload)
{
    m_ui->remoteComboBox->setRepository(m_repository);
    if (m_repository.isEmpty() || !QFileInfo(m_repository).isDir())
        return;
    *m_server = m_parameters->server;
    m_ui->remoteComboBox->updateRemotes(forceReload);
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
    const QModelIndex index = m_ui->treeView->selectionModel()->currentIndex();
    return index.isValid() ? m_filterModel->mapToSource(index) : QModelIndex();
}

void GerritDialog::updateButtons()
{
    const bool enabled = !m_fetchRunning && m_ui->treeView->selectionModel()->currentIndex().isValid();
    m_displayButton->setEnabled(enabled);
    m_cherryPickButton->setEnabled(enabled);
    m_checkoutButton->setEnabled(enabled);
}

void GerritDialog::slotCurrentChanged()
{
    const QModelIndex current = currentIndex();
    m_ui->detailsBrowser->setText(current.isValid() ? m_model->toHtml(current) : QString());
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
