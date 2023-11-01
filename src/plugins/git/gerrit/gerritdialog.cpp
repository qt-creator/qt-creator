// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gerritdialog.h"

#include "gerritmodel.h"
#include "gerritparameters.h"
#include "gerritremotechooser.h"

#include "../gitplugin.h"
#include "../gittr.h"

#include <coreplugin/icore.h>

#include <utils/fancylineedit.h>
#include <utils/hostosinfo.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
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

#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QSplitter>
#include <QTextBrowser>

using namespace Utils;

namespace Gerrit {
namespace Internal {

static const int maxTitleWidth = 350;

GerritDialog::GerritDialog(const QSharedPointer<GerritParameters> &p,
                           const QSharedPointer<GerritServer> &s,
                           const FilePath &repository,
                           QWidget *parent)
    : QDialog(parent)
    , m_parameters(p)
    , m_server(s)
    , m_filterModel(new QSortFilterProxyModel(this))
    , m_model(new GerritModel(p, this))
    , m_queryModel(new QStringListModel(this))
{
    setWindowTitle(Git::Tr::tr("Gerrit"));
    resize(950, 706);

    m_repositoryLabel = new QLabel(this);
    m_repositoryLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    m_remoteComboBox = new GerritRemoteChooser(this);
    m_remoteComboBox->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    m_remoteComboBox->setMinimumSize(QSize(40, 0));

    auto changesGroup = new QGroupBox(Git::Tr::tr("Changes"));
    changesGroup->setMinimumSize(QSize(0, 350));

    m_queryLineEdit = new FancyLineEdit(changesGroup);
    m_queryLineEdit->setMinimumSize(QSize(400, 0));
    m_queryLineEdit->setPlaceholderText(Git::Tr::tr("Change #, SHA-1, tr:id, owner:email or reviewer:email"));
    m_queryLineEdit->setSpecialCompleter(new QCompleter(m_queryModel, this));
    m_queryLineEdit->setValidationFunction(
        [this](FancyLineEdit *, QString *) { return m_model->state() != GerritModel::Error; });

    auto filterLineEdit = new FancyLineEdit(changesGroup);
    filterLineEdit->setMinimumSize(QSize(300, 0));
    filterLineEdit->setFiltering(true);

    m_treeView = new TreeView(changesGroup);
    m_treeView->setMinimumSize(QSize(600, 0));
    m_treeView->setRootIsDecorated(false);
    m_treeView->setSortingEnabled(true);

    auto detailsGroup = new QGroupBox(Git::Tr::tr("Details"));
    detailsGroup->setMinimumSize(QSize(0, 175));

    m_detailsBrowser = new QTextBrowser(detailsGroup);
    m_detailsBrowser->setOpenExternalLinks(true);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);

    auto queryLabel = new QLabel(Git::Tr::tr("&Query:"), changesGroup);
    queryLabel->setBuddy(m_queryLineEdit);

    m_remoteComboBox->setParameters(m_parameters);
    m_remoteComboBox->setFallbackEnabled(true);
    m_queryModel->setStringList(m_parameters->savedQueries);
    m_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filterModel->setSourceModel(m_model);
    m_filterModel->setFilterRole(GerritModel::FilterRole);
    m_filterModel->setSortRole(GerritModel::SortRole);

    m_treeView->setModel(m_filterModel);
    m_treeView->setActivationMode(Utils::DoubleClickActivation);

    m_progressIndicatorTimer.setSingleShot(true);
    m_progressIndicatorTimer.setInterval(50); // don't show progress for < 50ms tasks

    m_progressIndicator = new ProgressIndicator(ProgressIndicatorSize::Large, m_treeView);
    m_progressIndicator->attachToWidget(m_treeView->viewport());
    m_progressIndicator->hide();

    m_displayButton = addActionButton(Git::Tr::tr("&Show"), [this] { slotFetchDisplay(); });
    m_cherryPickButton = addActionButton(Git::Tr::tr("Cherry &Pick"), [this] { slotFetchCherryPick(); });
    m_checkoutButton = addActionButton(Git::Tr::tr("C&heckout"), [this] { slotFetchCheckout(); });
    m_refreshButton = addActionButton(Git::Tr::tr("&Refresh"), [this] { refresh(); });
    m_refreshButton->setDefault(true);

    using namespace Layouting;

    Column {
        Row {
            queryLabel,
            m_queryLineEdit,
            st,
            filterLineEdit
        },
        m_treeView
    }.attachTo(changesGroup);

    Column {
        m_detailsBrowser
    }.attachTo(detailsGroup);

    auto splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Vertical);
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(changesGroup);
    splitter->addWidget(detailsGroup);

    Column {
        Row { m_repositoryLabel, st, Git::Tr::tr("Remote:"),  m_remoteComboBox },
        splitter,
        m_buttonBox
    }.attachTo(this);

    connect(filterLineEdit, &Utils::FancyLineEdit::filterChanged,
            m_filterModel, &QSortFilterProxyModel::setFilterFixedString);
    connect(m_queryLineEdit, &QLineEdit::returnPressed,
            this, &GerritDialog::refresh);
    connect(m_model, &GerritModel::stateChanged,
            m_queryLineEdit, &Utils::FancyLineEdit::validate);
    connect(m_remoteComboBox, &GerritRemoteChooser::remoteChanged,
            this, &GerritDialog::remoteChanged);
    connect(&m_progressIndicatorTimer, &QTimer::timeout,
            this, [this] { setProgressIndicatorVisible(true); });
    connect(m_model, &GerritModel::stateChanged,
            this, &GerritDialog::manageProgressIndicator);

    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &GerritDialog::slotCurrentChanged);
    connect(m_treeView, &QAbstractItemView::activated,
            this, &GerritDialog::slotActivated);

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

    m_treeView->setFocus();

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

FilePath GerritDialog::repositoryPath() const
{
    return m_repository;
}

void GerritDialog::setCurrentPath(const FilePath &path)
{
    if (path == m_repository)
        return;
    m_repository = path;
    m_repositoryLabel->setText(Git::Internal::GitPlugin::msgRepositoryLabel(path));
    updateRemotes();
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

GerritDialog::~GerritDialog() = default;

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

void GerritDialog::refresh()
{
    const QString &query = m_queryLineEdit->text().trimmed();
    updateCompletions(query);
    m_model->refresh(m_server, query);
    m_treeView->sortByColumn(-1, Qt::DescendingOrder);
}

void GerritDialog::scheduleUpdateRemotes()
{
    if (isVisible())
        updateRemotes();
    else
        m_shouldUpdateRemotes = true;
}

void GerritDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (m_shouldUpdateRemotes) {
        m_shouldUpdateRemotes = false;
        updateRemotes();
    }
}

void GerritDialog::remoteChanged()
{
    const GerritServer server = m_remoteComboBox->currentServer();
    if (QSharedPointer<GerritServer> modelServer = m_model->server()) {
        if (*modelServer == server)
           return;
    }
    *m_server = server;
    if (isVisible())
        refresh();
}

void GerritDialog::updateRemotes(bool forceReload)
{
    m_remoteComboBox->setRepository(m_repository);
    if (m_repository.isEmpty() || !m_repository.isDir())
        return;
    *m_server = m_parameters->server;
    m_remoteComboBox->updateRemotes(forceReload);
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
    const QString toolTip = Git::Tr::tr("Fetching \"%1\"...").arg(change->title);
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
