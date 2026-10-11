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

#include <QApplication>
#include <QComboBox>
#include <QCompleter>
#include <QDesktopServices>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStringListModel>
#include <QTextBrowser>
#include <QUrl>

#ifdef WITH_TESTS
#include <QRegularExpression>
#include <QTest>
#endif

using namespace Utils;

namespace Gerrit::Internal {

static const int maxTitleWidth = 350;

GerritDialog::GerritDialog(const std::shared_ptr<GerritServer> &s,
                           const FilePath &repository,
                           QWidget *parent)
    : QDialog(parent)
    , m_server(s)
    , m_filterModel(new QSortFilterProxyModel(this))
    , m_model(new GerritModel(this))
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
    m_queryLineEdit->setObjectName("query");
    m_queryLineEdit->setMinimumSize(QSize(400, 0));
    m_queryLineEdit->setPlaceholderText(Git::Tr::tr("Change #, hash, tr:id, owner:email or reviewer:email"));
    m_queryLineEdit->setSpecialCompleter(new QCompleter(m_queryModel, this));
    m_queryLineEdit->setValidationFunction([this](const QString &) -> Result<> {
        if (m_model->state() != GerritModel::Error)
            return ResultOk;
        return ResultError(QString());
    });

    auto filterLineEdit = new FancyLineEdit(changesGroup);
    filterLineEdit->setObjectName("filter");
    filterLineEdit->setMinimumSize(QSize(300, 0));
    filterLineEdit->setFiltering(true);

    m_treeView = new TreeView(changesGroup);
    m_treeView->setObjectName("changes");
    m_treeView->setMinimumSize(QSize(600, 0));
    m_treeView->setRootIsDecorated(false);
    m_treeView->setSortingEnabled(true);

    auto detailsGroup = new QGroupBox(Git::Tr::tr("Details"));
    detailsGroup->setMinimumSize(QSize(0, 175));

    m_detailsBrowser = new QTextBrowser(detailsGroup);
    m_detailsBrowser->setOpenExternalLinks(true);
    m_patchSetComboBox = new QComboBox(detailsGroup);
    m_patchSetComboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_patchSetComboBox->setObjectName("GerritPatchSetComboBox"); // autotest
    auto patchSetLabel = new QLabel(Git::Tr::tr("Patch s&et:"), detailsGroup);
    patchSetLabel->setBuddy(m_patchSetComboBox);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    m_buttonBox->button(QDialogButtonBox::Close)->setAutoDefault(false);

    auto queryLabel = new QLabel(Git::Tr::tr("&Query:"), changesGroup);
    queryLabel->setBuddy(m_queryLineEdit);

    m_remoteComboBox->setFallbackEnabled(true);
    m_queryModel->setStringList(gerritSettings().savedQueries);
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
        m_detailsBrowser,
        Row { patchSetLabel, m_patchSetComboBox, st }
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
            this, [this](const QString &filter) {
        m_filterModel->setFilterFixedString(filter);
        m_treeView->expandAll();
    });
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
    connect(m_model, &QAbstractItemModel::modelReset,
            this, &GerritDialog::slotCurrentChanged);
    connect(m_treeView, &QAbstractItemView::activated,
            this, &GerritDialog::slotActivated);

    connect(m_model, &GerritModel::refreshStateChanged,
            m_refreshButton, &QWidget::setDisabled);
    connect(m_model, &GerritModel::refreshStateChanged,
            this, &GerritDialog::slotRefreshStateChanged);
    connect(m_model, &GerritModel::errorText,
            this, [this](const QString &text) {
        m_detailsBrowser->setPlainText(Git::Tr::tr("Gerrit query failed:\n%1").arg(text));
        if (text.contains("returned error: 401"))
            updateRemotes(true);
    }, Qt::QueuedConnection);

    connect(m_patchSetComboBox, &QComboBox::currentIndexChanged,
            this, &GerritDialog::updateDetails);

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
    m_repositoryLabel->setText(Git::Internal::msgRepositoryLabel(path));
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
    QStringList &queries = gerritSettings().savedQueries;
    queries.removeAll(query);
    queries.prepend(query);
    m_queryModel->setStringList(queries);
    gerritSettings().saveQueries();
}

GerritDialog::~GerritDialog() = default;

bool GerritDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        auto keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape && !keyEvent->modifiers()) {
            keyEvent->accept();
            return true;
        }
    }
    return QDialog::event(event);
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
    if (const auto change = selectedChange())
        emit fetchDisplay(change);
}

void GerritDialog::slotFetchCherryPick()
{
    if (const auto change = selectedChange())
        emit fetchCherryPick(change);
}

void GerritDialog::slotFetchCheckout()
{
    if (const auto change = selectedChange())
        emit fetchCheckout(change);
}

void GerritDialog::refresh()
{
    const QString &query = m_queryLineEdit->text().trimmed();
    updateCompletions(query);
    m_detailsBrowser->clear();
    m_model->refresh(m_server, query);
    m_treeView->sortByColumn(-1, Qt::AscendingOrder);
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
    if (std::shared_ptr<GerritServer> modelServer = m_model->server()) {
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
    *m_server = gerritSettings().server;
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
    const bool hasPatchSet = currentIndex().isValid() && m_patchSetComboBox->currentIndex() >= 0;
    m_patchSetComboBox->setEnabled(!m_fetchRunning && hasPatchSet);
    const bool enabled = !m_fetchRunning && hasPatchSet;
    m_displayButton->setEnabled(enabled);
    m_cherryPickButton->setEnabled(enabled);
    m_checkoutButton->setEnabled(enabled);
}

void GerritDialog::slotCurrentChanged()
{
    const QSignalBlocker blocker(m_patchSetComboBox);
    m_patchSetComboBox->clear();
    const QModelIndex current = currentIndex();
    if (current.isValid()) {
        const GerritChangePtr change = m_model->change(current);
        for (const GerritPatchSet &patchSet : change->patchSets) {
            const int number = patchSet.patchSetNumber;
            const QString text = number == change->currentPatchSet.patchSetNumber
                ? Git::Tr::tr("%1 (latest)").arg(number) : QString::number(number);
            m_patchSetComboBox->addItem(text, number);
        }
        m_patchSetComboBox->setCurrentIndex(
            m_patchSetComboBox->findData(change->currentPatchSet.patchSetNumber));
    }
    updateDetails();
    updateButtons();
}

void GerritDialog::updateDetails()
{
    const QModelIndex current = currentIndex();
    m_detailsBrowser->setText(current.isValid()
        ? m_model->toHtml(current, m_patchSetComboBox->currentData().toInt()) : QString());
}

GerritChangePtr GerritDialog::selectedChange() const
{
    const QModelIndex current = currentIndex();
    if (!current.isValid() || m_patchSetComboBox->currentIndex() < 0)
        return {};
    const GerritChangePtr change = m_model->change(current);
    const int number = m_patchSetComboBox->currentData().toInt();
    for (const GerritPatchSet &patchSet : change->patchSets) {
        if (patchSet.patchSetNumber == number) {
            auto selected = std::make_shared<GerritChange>(*change);
            selected->currentPatchSet = patchSet;
            return selected;
        }
    }
    return {};
}

void GerritDialog::fetchStarted(const QString &changeTitle)
{
    // Disable buttons to prevent parallel gerrit operations which can cause mix-ups.
    m_fetchRunning = true;
    updateButtons();
    const QString toolTip = Git::Tr::tr("Fetching \"%1\"...").arg(changeTitle);
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

#ifdef WITH_TESTS

class GerritDialogTest final : public QObject
{
    Q_OBJECT
private slots:
    void testPatchSetSelection();
};

void GerritDialogTest::testPatchSetSelection()
{
    auto server = std::make_shared<GerritServer>();
    GerritDialog dialog(server, {});
    QVERIFY(!dialog.m_patchSetComboBox->isEnabled());
    QVERIFY(!dialog.m_cherryPickButton->isEnabled());
    auto change = std::make_shared<GerritChange>();
    change->number = 42;
    change->title = "Test change";
    change->url = "https://review/42";
    change->project = "project";
    change->currentPatchSet.patchSetNumber = 3;
    change->currentPatchSet.ref = "refs/changes/42/42/3";
    GerritPatchSet earlier;
    earlier.patchSetNumber = 1;
    earlier.ref = "refs/changes/42/42/1";
    change->patchSets = {change->currentPatchSet, earlier};
    const auto appendChange = [&dialog](const GerritChangePtr &change) {
        QList<QStandardItem *> row;
        for (int i = 0; i < GerritModel::ColumnCount; ++i) {
            auto item = new QStandardItem;
            item->setData(QVariant::fromValue(change), GerritModel::GerritChangeRole);
            row.append(item);
        }
        dialog.m_model->appendRow(row);
    };
    appendChange(change);
    dialog.m_treeView->setCurrentIndex(dialog.m_filterModel->index(0, 0));
    QCOMPARE(dialog.m_patchSetComboBox->count(), 2);
    QCOMPARE(dialog.m_patchSetComboBox->currentData().toInt(), 3);
    QVERIFY(dialog.m_patchSetComboBox->isEnabled());
    QVERIFY(dialog.m_cherryPickButton->isEnabled());
    QVERIFY(dialog.m_detailsBrowser->toPlainText().contains(QRegularExpression("Patch set\\s+3")));

    QList<GerritChangePtr> fetched;
    const auto capture = [&fetched](const GerritChangePtr &selected) { fetched.append(selected); };
    connect(&dialog, &GerritDialog::fetchDisplay, &dialog, capture);
    connect(&dialog, &GerritDialog::fetchCherryPick, &dialog, capture);
    connect(&dialog, &GerritDialog::fetchCheckout, &dialog, capture);
    dialog.m_patchSetComboBox->setCurrentIndex(dialog.m_patchSetComboBox->findData(1));
    QVERIFY(dialog.m_detailsBrowser->toPlainText().contains(QRegularExpression("Patch set\\s+1")));
    QVERIFY(dialog.m_detailsBrowser->toHtml().contains("href=\"https://review/42/1\""));
    dialog.m_displayButton->click();
    dialog.m_cherryPickButton->click();
    dialog.m_checkoutButton->click();
    QCOMPARE(fetched.size(), 3);
    for (const GerritChangePtr &selected : fetched) {
        QVERIFY(selected != change);
        QCOMPARE(selected->currentPatchSet.patchSetNumber, 1);
        QCOMPARE(selected->gitFetchArguments(*server).last(), earlier.ref);
    }
    QCOMPARE(change->currentPatchSet.patchSetNumber, 3);
    dialog.fetchStarted(change->title);
    QVERIFY(!dialog.m_patchSetComboBox->isEnabled());
    QVERIFY(!dialog.m_cherryPickButton->isEnabled());
    dialog.fetchFinished();
    QVERIFY(dialog.m_patchSetComboBox->isEnabled());

    auto another = std::make_shared<GerritChange>(*change);
    another->patchSets = {another->currentPatchSet};
    appendChange(another);
    dialog.m_treeView->setCurrentIndex(dialog.m_filterModel->index(1, 0));
    QCOMPARE(dialog.m_patchSetComboBox->count(), 1);
    QCOMPARE(dialog.m_patchSetComboBox->currentData().toInt(), 3);
    dialog.m_cherryPickButton->click();
    QCOMPARE(fetched.last()->currentPatchSet.patchSetNumber, 3);
    QCOMPARE(fetched.first()->currentPatchSet.patchSetNumber, 1);
    dialog.m_model->clear();
    QCOMPARE(dialog.m_patchSetComboBox->count(), 0);
    QVERIFY(!dialog.m_patchSetComboBox->isEnabled());
    QVERIFY(!dialog.m_displayButton->isEnabled());
    QVERIFY(!dialog.m_cherryPickButton->isEnabled());
    QVERIFY(!dialog.m_checkoutButton->isEnabled());
}

QObject *createGerritDialogTest()
{
    return new GerritDialogTest;
}

#endif

} // Gerrit::Internal

#ifdef WITH_TESTS
#include "gerritdialog.moc"
#endif
