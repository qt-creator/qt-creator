// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionperspective.h"

#include "axivionplugin.h"
#include "axivionsettings.h"
#include "axiviontr.h"
#include "dashboard/dto.h"
#include "issueheaderview.h"
#include "dynamiclistmodel.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <debugger/debuggerconstants.h>
#include <debugger/debuggermainwindow.h>

#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <solutions/tasking/conditional.h>
#include <solutions/tasking/tasktreerunner.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/checkablemessagebox.h>
#include <utils/guard.h>
#include <utils/layoutbuilder.h>
#include <utils/link.h>
#include <utils/qtcassert.h>
#include <utils/basetreeview.h>
#include <utils/utilsicons.h>
#include <utils/overlaywidget.h>

#include <QButtonGroup>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStandardItemModel>
#include <QTextBrowser>
#include <QToolButton>
#include <QUrlQuery>

#include <map>

using namespace Core;
using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace Axivion::Internal {

static const Icon MARKER_ICON({{":/axivion/images/marker.png", Theme::IconsBaseColor}});

static QPixmap trendIcon(qint64 added, qint64 removed)
{
    static const QPixmap unchanged = Utils::Icons::NEXT.pixmap();
    static const QPixmap increased = Icon(
                { {":/utils/images/arrowup.png", Theme::IconsErrorColor} }).pixmap();
    static const QPixmap decreased = Icon(
                {  {":/utils/images/arrowdown.png", Theme::IconsRunColor} }).pixmap();
    if (added == removed)
        return unchanged;
    return added < removed ? decreased : increased;
}

struct LinkWithColumns
{
    Link link;
    QList<int> columns;
};

static bool issueListContextMenuEvent(const ItemViewEvent &ev); // impl at bottom

static std::optional<PathMapping> findPathMappingMatch(const QString &projectName,
                                                       const Link &link)
{
    QTC_ASSERT(!projectName.isEmpty(), return std::nullopt);
    for (const PathMapping &mapping : settings().validPathMappings()) {
        if (mapping.projectName != projectName)
            continue;

        if (mapping.analysisPath.isEmpty())
            return mapping;

        QString analysis = mapping.analysisPath.toString();
        // ensure we use complete paths
        if (!analysis.endsWith('/'))
            analysis.append('/');
        if (!link.targetFilePath.startsWith(analysis))
            continue;

        return mapping;
    }
    return std::nullopt;
}

class IssueListItem final : public ListItem
{
public:
    IssueListItem(int row, const QString &id, const QStringList &data, const QStringList &toolTips)
        : ListItem(row)
        , m_id(id)
        , m_data(data)
        , m_toolTips(toolTips)
    {}

    void setLinks(const QList<LinkWithColumns> &links) { m_links = links; }

    QVariant data(int column, int role) const
    {
        if (role == Qt::DisplayRole && column >= 0 && column < m_data.size())
            return m_data.at(column);
        if (role == Qt::ToolTipRole && column >= 0 && column < m_toolTips.size())
            return m_toolTips.at(column);
        return {};
    }

    bool setData(int column, const QVariant &value, int role) final
    {
        if (role == BaseTreeView::ItemActivatedRole) {
            if (!m_links.isEmpty()) {
                Link link
                        = Utils::findOr(m_links, m_links.first(), [column](const LinkWithColumns &link) {
                    return link.columns.contains(column);
                }).link;

                // get the file path either by the open project(s) or by using the path mapping
                // prefer to use the open project's file instead of some generic approach
                const FilePath computedPath = findFileForIssuePath(link.targetFilePath);
                FilePath targetFilePath;
                if (!computedPath.exists()) {
                    if (const std::optional<Dto::ProjectInfoDto> pInfo = projectInfo()) {
                        if (auto mapping = findPathMappingMatch(pInfo->name, link)) {
                            std::optional<FilePath> fp = link.targetFilePath.prefixRemoved(
                                        mapping->analysisPath.toString());
                            QTC_CHECK(fp);
                            fp = mapping->localPath.pathAppended(fp->toString());
                            if (fp->exists())
                                targetFilePath = *fp;
                        }
                    }
                }
                link.targetFilePath = targetFilePath.isEmpty() ? computedPath : targetFilePath;
                if (link.targetFilePath.exists())
                    EditorManager::openEditorAt(link);
            }
            if (!m_id.isEmpty())
                fetchIssueInfo(m_id);
            return true;
        } else if (role == BaseTreeView::ItemViewEventRole && !m_id.isEmpty()) {
            ItemViewEvent ev = value.value<ItemViewEvent>();
            if (ev.as<QContextMenuEvent>())
                return issueListContextMenuEvent(ev);
        }
        return ListItem::setData(column, value, role);
    }

private:
    const QString m_id;
    const QStringList m_data;
    const QStringList m_toolTips;
    QList<LinkWithColumns> m_links;
};

class IssuesWidget : public QScrollArea
{
public:
    explicit IssuesWidget(QWidget *parent = nullptr);
    void updateUi(const QString &kind);
    void initDashboardList(const QString &preferredProject = {});
    void resetDashboard();

    const std::optional<Dto::TableInfoDto> currentTableInfo() const { return m_currentTableInfo; }
    IssueListSearch searchFromUi() const;

    enum OverlayIconType { EmptyIcon, ErrorIcon, SettingsIcon };
    void showOverlay(const QString &message = {}, OverlayIconType type = EmptyIcon);
    void showErrorMessage(const QString &message);
protected:
    void showEvent(QShowEvent *event) override;
private:
    void reinitProjectList(const QString &currentProject);
    void updateTable();
    void addIssues(const Dto::IssueTableDto &dto, int startRow);
    void onSearchParameterChanged();
    void updateVersionItemsEnabledState();
    void updateBasicProjectInfo(const std::optional<Dto::ProjectInfoDto> &info);
    void setFiltersEnabled(bool enabled);
    void fetchTable();
    void fetchIssues(const IssueListSearch &search);
    void onFetchRequested(int startRow, int limit);
    void hideOverlays();

    QString m_currentPrefix;
    QString m_currentProject;
    std::optional<Dto::TableInfoDto> m_currentTableInfo;
    QHBoxLayout *m_typesLayout = nullptr;
    QButtonGroup *m_typesButtonGroup = nullptr;
    QPushButton *m_addedFilter = nullptr;
    QPushButton *m_removedFilter = nullptr;
    QComboBox *m_dashboards = nullptr;
    QComboBox *m_dashboardProjects = nullptr;
    QComboBox *m_ownerFilter = nullptr;
    QComboBox *m_versionStart = nullptr;
    QComboBox *m_versionEnd = nullptr;
    Guard m_signalBlocker;
    QLineEdit *m_pathGlobFilter = nullptr; // FancyLineEdit instead?
    QLabel *m_totalRows = nullptr;
    BaseTreeView *m_issuesView = nullptr;
    QStackedWidget *m_stack = nullptr;
    IssueHeaderView *m_headerView = nullptr;
    QPlainTextEdit *m_errorEdit = nullptr;
    DynamicListModel *m_issuesModel = nullptr;
    int m_totalRowCount = 0;
    QStringList m_userNames;
    QStringList m_versionDates;
    TaskTreeRunner m_taskTreeRunner;
    OverlayWidget *m_overlay = nullptr;
    bool m_dashboardListUninitialized = true;
};

IssuesWidget::IssuesWidget(QWidget *parent)
    : QScrollArea(parent)
{
    setFrameStyle(QFrame::NoFrame);
    QWidget *widget = new QWidget(this);
    m_dashboards = new QComboBox(this);
    m_dashboards->setMinimumContentsLength(15);
    connect(m_dashboards, &QComboBox::currentIndexChanged, this, [this] {
        if (m_signalBlocker.isLocked())
            return;
        const QVariant data = m_dashboards->currentData();
        if (data.isValid()) {
            const AxivionServer server = data.value<AxivionServer>();
            switchActiveDashboardId(server.id);
            reinitProjectList(m_dashboardProjects->currentText());
        } else {
            switchActiveDashboardId({});
            {
                GuardLocker lock(m_signalBlocker);
                m_dashboardProjects->clear();
            }
            updateBasicProjectInfo(std::nullopt);
            m_issuesView->hideProgressIndicator();
        }
    });

    m_dashboardProjects = new QComboBox(this);
    m_dashboardProjects->setMinimumContentsLength(25);
    connect(m_dashboardProjects, &QComboBox::currentIndexChanged, this, [this] {
        if (m_signalBlocker.isLocked())
            return;
        m_currentPrefix.clear();
        m_currentProject.clear();
        m_issuesModel->clear();
        fetchDashboardAndProjectInfo({}, m_dashboardProjects->currentText());
    });
    // row with issue types (-> depending on choice, tables below change)
    //  and a selectable range (start version, end version)
    // row with added/removed and some filters (assignee, path glob, (named filter))
    // table, columns depend on chosen issue type
    m_typesButtonGroup = new QButtonGroup(this);
    m_typesButtonGroup->setExclusive(true);
    m_typesLayout = new QHBoxLayout;

    m_versionStart = new QComboBox(this);
    m_versionStart->setMinimumContentsLength(25);
    connect(m_versionStart, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (m_signalBlocker.isLocked())
            return;
        QTC_ASSERT(index > -1 && index < m_versionDates.size(), return);
        updateVersionItemsEnabledState();
        onSearchParameterChanged();
    });

    m_versionEnd = new QComboBox(this);
    m_versionEnd->setMinimumContentsLength(25);
    connect(m_versionEnd, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (m_signalBlocker.isLocked())
            return;
        QTC_ASSERT(index > -1 && index < m_versionDates.size(), return);
        updateVersionItemsEnabledState();
        onSearchParameterChanged();
        setAnalysisVersion(m_versionDates.at(index));
    });

    m_addedFilter = new QPushButton(this);
    m_addedFilter->setIcon(trendIcon(1, 0));
    m_addedFilter->setText("0");
    m_addedFilter->setCheckable(true);
    connect(m_addedFilter, &QPushButton::clicked, this, [this](bool checked) {
        if (checked && m_removedFilter->isChecked())
            m_removedFilter->setChecked(false);
        onSearchParameterChanged();
    });

    m_removedFilter = new QPushButton(this);
    m_removedFilter->setIcon(trendIcon(0, 1));
    m_removedFilter->setText("0");
    m_removedFilter->setCheckable(true);
    connect(m_removedFilter, &QPushButton::clicked, this, [this](bool checked) {
        if (checked && m_addedFilter->isChecked())
            m_addedFilter->setChecked(false);
        onSearchParameterChanged();
    });

    m_ownerFilter = new QComboBox(this);
    m_ownerFilter->setToolTip(Tr::tr("Owner"));
    m_ownerFilter->setMinimumContentsLength(25);
    connect(m_ownerFilter, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (m_signalBlocker.isLocked())
            return;
        QTC_ASSERT(index > -1 && index < m_userNames.size(), return);
        onSearchParameterChanged();
    });

    m_pathGlobFilter = new QLineEdit(this);
    m_pathGlobFilter->setPlaceholderText(Tr::tr("Path globbing"));
    connect(m_pathGlobFilter, &QLineEdit::textEdited, this, &IssuesWidget::onSearchParameterChanged);

    m_issuesView = new BaseTreeView(this);
    m_issuesView->setFrameShape(QFrame::StyledPanel); // Bring back Qt default
    m_issuesView->setFrameShadow(QFrame::Sunken);     // Bring back Qt default
    m_headerView = new IssueHeaderView(this);
    m_headerView->setSectionsMovable(true);
    connect(m_headerView, &IssueHeaderView::sortTriggered,
            this, &IssuesWidget::onSearchParameterChanged);
    connect(m_headerView, &IssueHeaderView::filterChanged,
            this, &IssuesWidget::onSearchParameterChanged);
    m_issuesView->setHeader(m_headerView);
    m_issuesView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_issuesView->enableColumnHiding();
    m_issuesModel = new DynamicListModel(this);
    m_issuesView->setModel(m_issuesModel);
    connect(m_issuesModel, &DynamicListModel::fetchRequested, this, &IssuesWidget::onFetchRequested);
    m_totalRows = new QLabel(Tr::tr("Total rows:"), this);

    QWidget *errorWidget = new QWidget(this);
    m_errorEdit = new QPlainTextEdit(this);
    m_errorEdit->setReadOnly(true);
    QPalette palette = Utils::creatorTheme()->palette();
    palette.setColor(QPalette::Text, Utils::creatorColor(Theme::TextColorError));
    m_errorEdit->setPalette(palette);
    QPushButton *openPref = new QPushButton(Tr::tr("Open Preferences..."), errorWidget);
    connect(openPref, &QPushButton::clicked,
            this, []{ ICore::showOptionsDialog("Axivion.Settings.General"); });
    using namespace Layouting;
    Column {
        m_errorEdit,
        Row { openPref, st }
    }.attachTo(errorWidget);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_issuesView);
    m_stack->addWidget(errorWidget);

    Column {
        Row { m_dashboards, m_dashboardProjects, empty, m_typesLayout, st, m_versionStart, m_versionEnd, st },
        Row { m_addedFilter, m_removedFilter, Space(1), m_ownerFilter, m_pathGlobFilter },
        m_stack,
        Row { st, m_totalRows }
    }.attachTo(widget);

    setWidget(widget);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setWidgetResizable(true);

    connect(&settings(), &AxivionSettings::changed,
            this, [this] { initDashboardList(); });
}

void IssuesWidget::updateUi(const QString &kind)
{
    setFiltersEnabled(false);
    const std::optional<Dto::ProjectInfoDto> projectInfo = Internal::projectInfo();
    updateBasicProjectInfo(projectInfo);

    if (!projectInfo)
        return;
    const Dto::ProjectInfoDto &info = *projectInfo;
    if (info.versions.empty()) // add some warning/information?
        return;

    setFiltersEnabled(true);
    // avoid refetching existing data
    if (kind.isEmpty() && (!m_currentPrefix.isEmpty() || m_issuesModel->rowCount()))
        return;

    if (!kind.isEmpty()) {
        const int index
                = Utils::indexOf( info.issueKinds, [kind](const Dto::IssueKindInfoDto &dto) {
            return dto.prefix == kind; });
        if (index != -1) {
            m_currentPrefix = kind;
            auto kindButton = m_typesButtonGroup->button(index + 1);
            if (QTC_GUARD(kindButton))
                kindButton->setChecked(true);
            // reset filters - if kind is not empty we get triggered from dashboard overview
            if (!m_userNames.isEmpty())
                m_ownerFilter->setCurrentIndex(0);
            m_pathGlobFilter->clear();
            if (m_versionDates.size() > 1) {
                m_versionStart->setCurrentIndex(m_versionDates.count() - 1);
                m_versionEnd->setCurrentIndex(0);
            }
        }
    }
    if (m_currentPrefix.isEmpty())
        m_currentPrefix = info.issueKinds.size() ? info.issueKinds.front().prefix : QString{};
    fetchTable();
}

void IssuesWidget::resetDashboard()
{
    setFiltersEnabled(false);
    updateBasicProjectInfo(std::nullopt);
    GuardLocker lock(m_signalBlocker);
    m_dashboardProjects->clear();
    m_dashboards->clear();
    m_dashboardListUninitialized = true;
}

void IssuesWidget::initDashboardList(const QString &preferredProject)
{
    const QString currentProject = preferredProject.isEmpty() ? m_dashboardProjects->currentText()
                                                              : preferredProject;
    resetDashboard();
    m_dashboardListUninitialized = false;
    const QList<AxivionServer> servers = settings().allAvailableServers();
    if (servers.isEmpty()) {
        switchActiveDashboardId({});
        showOverlay(Tr::tr("Configure dashboards in Preferences > Axivion > General."), SettingsIcon);
        return;
    }
    hideOverlays();

    GuardLocker lock(m_signalBlocker);
    m_dashboards->addItem(Tr::tr("No Dashboard"));
    for (const AxivionServer &server : servers)
        m_dashboards->addItem(server.displayString(), QVariant::fromValue(server));

    Id activeId = activeDashboardId();
    if (activeId.isValid()) {
        int index = Utils::indexOf(servers, Utils::equal(&AxivionServer::id, activeId));
        if (index < 0) {
            activeId = settings().defaultDashboardId();
            index = Utils::indexOf(servers, Utils::equal(&AxivionServer::id, activeId));
        }
        m_dashboards->setCurrentIndex(index + 1);
        reinitProjectList(currentProject);
    } else {
        m_dashboards->setCurrentIndex(0);
    }
}

void IssuesWidget::reinitProjectList(const QString &currentProject)
{
    const auto onDashboardInfoFetched
            = [this, currentProject] (const expected_str<DashboardInfo> &info) {
        if (!info) {
            m_issuesView->hideProgressIndicator();
            return;
        }
        GuardLocker lock(m_signalBlocker);
        m_dashboardProjects->addItems(info->projects);
        if (!currentProject.isEmpty() && info->projects.contains(currentProject))
            m_dashboardProjects->setCurrentText(currentProject);
    };
    {
        GuardLocker lock(m_signalBlocker);
        m_dashboardProjects->clear();
    }
    updateBasicProjectInfo(std::nullopt);
    hideOverlays();
    m_issuesView->showProgressIndicator();
    fetchDashboardAndProjectInfo(onDashboardInfoFetched, currentProject);
}

static Qt::Alignment alignmentFromString(const QString &str)
{
    if (str == "left")
        return Qt::AlignLeft;
    if (str == "right")
        return Qt::AlignRight;
    if (str == "center")
        return Qt::AlignHCenter;
    return Qt::AlignLeft;
}

void IssuesWidget::showEvent(QShowEvent *event)
{
    if (m_dashboardListUninitialized)
        initDashboardList();
    QWidget::showEvent(event);
}

void IssuesWidget::updateTable()
{
    if (!m_currentTableInfo)
        return;

    QStringList columnHeaders;
    QStringList hiddenColumns;
    QList<IssueHeaderView::ColumnInfo> columnInfos;
    QList<Qt::Alignment> alignments;
    for (const Dto::ColumnInfoDto &column : m_currentTableInfo->columns) {
        columnHeaders << column.header.value_or(column.key);
        if (!column.showByDefault)
            hiddenColumns << columnHeaders.last();
        IssueHeaderView::ColumnInfo info;
        info.key = column.key;
        info.sortable = column.canSort;
        info.filterable = column.canFilter;
        info.width = column.width;
        columnInfos.append(info);
        alignments << alignmentFromString(column.alignment);
    }
    m_addedFilter->setText("0");
    m_removedFilter->setText("0");
    m_totalRows->setText(Tr::tr("Total rows:"));

    m_issuesModel->clear();
    m_issuesModel->setHeader(columnHeaders);
    m_issuesModel->setAlignments(alignments);
    m_headerView->setColumnInfoList(columnInfos);
    int counter = 0;
    for (const QString &header : std::as_const(columnHeaders))
        m_issuesView->setColumnHidden(counter++, hiddenColumns.contains(header));
    m_headerView->resizeSections(QHeaderView::ResizeToContents);
}

static QList<LinkWithColumns> linksForIssue(const std::map<QString, Dto::Any> &issueRow,
                                            const std::vector<Dto::ColumnInfoDto> &columnInfos)
{
    QList<LinkWithColumns> links;

    auto end = issueRow.end();
    auto findColumn = [columnInfos](const QString &columnKey) {
        int col = 0;
        for (auto it = columnInfos.cbegin(), end = columnInfos.cend(); it != end; ++it) {
            if (it->key == columnKey)
                return col;
            ++col;
        }
        return -1;
    };
    auto findAndAppend = [&links, &issueRow, &findColumn, &end](const QString &path,
            const QString &line) {
        QList<int> columns;
        auto it = issueRow.find(path);
        if (it != end && !it->second.isNull()) {
            QTC_ASSERT(it->second.isString(), return);
            Link link{ FilePath::fromUserInput(it->second.getString()) };
            columns.append(findColumn(it->first));
            it = issueRow.find(line);
            if (it != end && !it->second.isNull()) {
                QTC_ASSERT(it->second.isDouble(), return);
                link.targetLine = it->second.getDouble();
                columns.append(findColumn(it->first));
            }
            links.append({link, columns});
        }
    };
    // do these always? or just for their "expected" kind
    findAndAppend("path", "line");
    findAndAppend("sourcePath", "sourceLine");
    findAndAppend("targetPath", "targetLine");
    findAndAppend("leftPath", "leftLine");
    findAndAppend("rightPath", "rightLine");

    return links;
}

void IssuesWidget::addIssues(const Dto::IssueTableDto &dto, int startRow)
{
    QTC_ASSERT(m_currentTableInfo.has_value(), return);
    if (dto.totalRowCount.has_value()) {
        m_totalRowCount = *dto.totalRowCount;
        m_issuesModel->setExpectedRowCount(m_totalRowCount);
        m_totalRows->setText(Tr::tr("Total rows:") + ' ' + QString::number(m_totalRowCount));
    }
    if (dto.totalAddedCount.has_value())
        m_addedFilter->setText(QString::number(*dto.totalAddedCount));
    if (dto.totalRemovedCount.has_value())
        m_removedFilter->setText(QString::number(*dto.totalRemovedCount));

    const std::vector<Dto::ColumnInfoDto> &tableColumns = m_currentTableInfo->columns;
    const std::vector<std::map<QString, Dto::Any>> &rows = dto.rows;
    QList<ListItem *> items;
    for (const auto &row : rows) {
        QString id;
        QStringList data;
        QStringList toolTips;
        for (const auto &column : tableColumns) {
            const auto it = row.find(column.key);
            if (it != row.end()) {
                QString value = anyToSimpleString(it->second);
                if (column.key == "id") {
                    value.prepend(m_currentPrefix);
                    id = value;
                }
                toolTips << value;
                if (column.key.toLower().endsWith("path")) {
                    const FilePath fp = FilePath::fromUserInput(value);
                    value = QString("%1 [%2]").arg(fp.fileName(), fp.path());
                }
                data << value;
            }
        }
        IssueListItem *it = new IssueListItem(startRow++, id, data, toolTips);
        it->setLinks(linksForIssue(row, tableColumns));
        items.append(it);
    }
    m_issuesModel->setItems(items);
    if (items.isEmpty() && m_totalRowCount == 0)
        showOverlay();
}

void IssuesWidget::onSearchParameterChanged()
{
    m_addedFilter->setText("0");
    m_removedFilter->setText("0");
    m_totalRows->setText(Tr::tr("Total rows:"));

    m_issuesModel->clear();
    // new "first" time lookup
    m_totalRowCount = 0;
    IssueListSearch search = searchFromUi();
    search.computeTotalRowCount = true;
    fetchIssues(search);
}

void IssuesWidget::updateVersionItemsEnabledState()
{
    const int versionCount = m_versionDates.size();
    if (versionCount < 2)
        return;

    const int currentStart = m_versionStart->currentIndex();
    const int currentEnd = m_versionEnd->currentIndex();
    // Note: top-most item == index 0; bottom-most item == last / highest index
    QTC_ASSERT(currentStart > currentEnd, return);

    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(m_versionStart->model());
    QTC_ASSERT(model, return);
    for (int i = 0; i < versionCount; ++i) {
        if (auto item = model->item(i))
            item->setEnabled(i > currentEnd);
    }
    model = qobject_cast<QStandardItemModel *>(m_versionEnd->model());
    QTC_ASSERT(model, return);
    for (int i = 0; i < versionCount; ++i) {
        if (auto item = model->item(i))
            item->setEnabled(i < currentStart);
    }
}

void IssuesWidget::updateBasicProjectInfo(const std::optional<Dto::ProjectInfoDto> &info)
{
    auto cleanOld = [this] {
        const QList<QAbstractButton *> originalList = m_typesButtonGroup->buttons();
        QLayoutItem *child;
        while ((child = m_typesLayout->takeAt(0)) != nullptr) {
            if (originalList.contains(child->widget()))
                m_typesButtonGroup->removeButton(qobject_cast<QAbstractButton *>(child->widget()));
            delete child->widget();
            delete child;
        }
    };

    if (!info) {
        cleanOld();
        GuardLocker lock(m_signalBlocker);
        m_userNames.clear();
        m_versionDates.clear();
        m_ownerFilter->clear();
        m_versionStart->clear();
        m_versionEnd->clear();
        m_pathGlobFilter->clear();

        m_currentProject.clear();
        m_currentPrefix.clear();
        m_totalRowCount = 0;
        m_addedFilter->setText("0");
        m_removedFilter->setText("0");
        setFiltersEnabled(false);
        m_issuesModel->clear();
        m_issuesModel->setHeader({});
        hideOverlays();
        return;
    }

    if (m_currentProject == info->name)
        return;
    m_currentProject = info->name;

    cleanOld();

    const std::vector<Dto::IssueKindInfoDto> &issueKinds = info->issueKinds;
    int buttonId = 0;
    for (const Dto::IssueKindInfoDto &kind : issueKinds) {
        auto button = new QToolButton(this);
        button->setIcon(iconForIssue(kind.getOptionalPrefixEnum()));
        button->setToolTip(kind.nicePluralName);
        button->setCheckable(true);
        connect(button, &QToolButton::clicked, this, [this, prefix = kind.prefix]{
            m_currentPrefix = prefix;
            fetchTable();
        });
        m_typesButtonGroup->addButton(button, ++buttonId);
        m_typesLayout->addWidget(button);
    }
    if (auto firstButton = m_typesButtonGroup->button(1))
        firstButton->setChecked(true);

    GuardLocker lock(m_signalBlocker);
    m_userNames.clear();
    m_ownerFilter->clear();
    QStringList userDisplayNames;
    for (const Dto::UserRefDto &user : info->users) {
        userDisplayNames.append(user.displayName);
        m_userNames.append(user.name);
    }
    m_ownerFilter->addItems(userDisplayNames);

    m_versionDates.clear();
    m_versionStart->clear();
    m_versionEnd->clear();
    QStringList versionLabels;
    const std::vector<Dto::AnalysisVersionDto> &versions = info->versions;
    for (auto it = versions.crbegin(); it != versions.crend(); ++it) {
        const Dto::AnalysisVersionDto &version = *it;
        versionLabels.append(version.name);
        m_versionDates.append(version.date);
    }
    m_versionStart->addItems(versionLabels);
    m_versionEnd->addItems(versionLabels);
    m_versionStart->setCurrentIndex(m_versionDates.count() - 1);
    updateVersionItemsEnabledState();
}

void IssuesWidget::setFiltersEnabled(bool enabled)
{
    m_addedFilter->setEnabled(enabled);
    m_removedFilter->setEnabled(enabled);
    m_ownerFilter->setEnabled(enabled);
    m_versionStart->setEnabled(enabled);
    m_versionEnd->setEnabled(enabled);
    m_pathGlobFilter->setEnabled(enabled);
}

IssueListSearch IssuesWidget::searchFromUi() const
{
    IssueListSearch search;
    QTC_ASSERT(m_currentTableInfo, return search);
    const int userIndex = m_ownerFilter->currentIndex();
    QTC_ASSERT(userIndex >= 0 && m_userNames.size() > userIndex, return search);
    const int versionStartIndex = m_versionStart->currentIndex();
    QTC_ASSERT(versionStartIndex >= 0 && m_versionDates.size() > versionStartIndex, return search);
    const int versionEndIndex = m_versionEnd->currentIndex();
    QTC_ASSERT(versionEndIndex >= 0 && m_versionDates.size() > versionEndIndex, return search);
    search.kind = m_currentPrefix; // not really ui.. but anyhow
    search.owner = m_userNames.at(userIndex);
    search.filter_path = m_pathGlobFilter->text();
    search.versionStart = m_versionDates.at(versionStartIndex);
    search.versionEnd = m_versionDates.at(versionEndIndex);
    // different approach: checked means disabling in webview, checked here means explicitly request
    // the checked one, having both checked is impossible (having none checked means fetch both)
    // reason for different approach: currently poor reflected inside the ui (TODO)
    if (m_addedFilter->isChecked())
        search.state = "added";
    else if (m_removedFilter->isChecked())
        search.state = "removed";

    search.sort = m_headerView->currentSortString();
    search.filter = m_headerView->currentFilterMapping();
    return search;
}

void IssuesWidget::fetchTable()
{
    QTC_ASSERT(!m_currentPrefix.isEmpty(), return);
    // fetch table dto and apply, on done fetch first data for the selected issues
    const auto tableHandler = [this](const Dto::TableInfoDto &dto) {
        m_currentTableInfo.emplace(dto);
    };
    const auto setupHandler = [this](TaskTree *) {
        m_totalRowCount = 0;
        m_currentTableInfo.reset();
        m_issuesView->showProgressIndicator();
    };
    const auto doneHandler = [this](DoneWith result) {
        if (result == DoneWith::Error) {
            m_issuesView->hideProgressIndicator();
            return;
        }
        // first time lookup... should we cache and maybe represent old data?
        updateTable();
        IssueListSearch search = searchFromUi();
        search.computeTotalRowCount = true;
        fetchIssues(search);
    };
    m_taskTreeRunner.start(tableInfoRecipe(m_currentPrefix, tableHandler), setupHandler, doneHandler);
}

void IssuesWidget::fetchIssues(const IssueListSearch &search)
{
    hideOverlays();
    const auto issuesHandler = [this, startRow = search.offset](const Dto::IssueTableDto &dto) {
        addIssues(dto, startRow);
    };
    const auto setupHandler = [this](TaskTree *) { m_issuesView->showProgressIndicator(); };
    const auto doneHandler = [this](DoneWith) { m_issuesView->hideProgressIndicator(); };
    m_taskTreeRunner.start(issueTableRecipe(search, issuesHandler), setupHandler, doneHandler);
}

void IssuesWidget::onFetchRequested(int startRow, int limit)
{
    if (m_taskTreeRunner.isRunning())
        return;

    IssueListSearch search = searchFromUi();
    search.offset = startRow;
    search.limit = limit;
    fetchIssues(search);
}

void IssuesWidget::showOverlay(const QString &message, OverlayIconType type)
{
    if (!m_overlay) {
        QTC_ASSERT(m_issuesView, return);
        m_overlay = new OverlayWidget(this);
        m_overlay->attachToWidget(m_issuesView);
    }

    m_overlay->setPaintFunction([message, type](QWidget *that, QPainter &p, QPaintEvent *) {
        static const QIcon noData = Icon({{":/axivion/images/nodata.png", Theme::IconsDisabledColor}},
                                         Utils::Icon::Tint).icon();
        static const QIcon error = Icon({{":/axivion/images/error.png", Theme::IconsErrorColor}},
                                        Utils::Icon::Tint).icon();
        static const QIcon settings = Icon({{":/utils/images/settings.png", Theme::IconsDisabledColor}},
                                           Utils::Icon::Tint).icon();
        QRect iconRect(0, 0, 32, 32);
        iconRect.moveCenter(that->rect().center());
        if (type == EmptyIcon)
            noData.paint(&p, iconRect);
        else if (type == ErrorIcon)
            error.paint(&p, iconRect);
        else if (type == SettingsIcon)
            settings.paint(&p, iconRect);
        p.save();
        p.setPen(Utils::creatorColor(Theme::TextColorDisabled));
        const QFontMetrics &fm = p.fontMetrics();
        p.drawText(iconRect.bottomRight() + QPoint{10, fm.height() / 2 - 16 - fm.descent()},
                   message.isEmpty() ? Tr::tr("No Data") : message);
        p.restore();
    });

    m_stack->setCurrentIndex(0);
    m_overlay->show();
}

void IssuesWidget::showErrorMessage(const QString &message)
{
    m_errorEdit->setPlainText(message);
    m_stack->setCurrentIndex(1);
}

void IssuesWidget::hideOverlays()
{
    if (m_overlay)
        m_overlay->hide();
    m_stack->setCurrentIndex(0);
}

static void loadImage(QPromise<QImage> &promise, const QByteArray &data)
{
    promise.addResult(QImage::fromData(data));
}

class LazyImageBrowser : public QTextBrowser
{
public:
    QVariant loadResource(int type, const QUrl &name) override
    {
        if (type == QTextDocument::ImageResource) {
            if (!m_loadingQueue.contains(name)) {
                m_loadingQueue.append(name);
                if (!m_loaderTaskTree.isRunning())
                    m_loaderTaskTree.start(m_recipe);
            }
            return QImage();
        }
        return QTextBrowser::loadResource(type, name);
    }
private:
    Group recipe() {
        const LoopUntil iterator([this](int) { return !m_loadingQueue.isEmpty(); });

        const Storage<DownloadData> storage;

        const auto onSetup = [this, storage] {
            storage->inputUrl = resolveDashboardInfoUrl(m_loadingQueue.first());
            storage->expectedContentType = ContentType::Svg;
        };

        const auto onImageLoadSetup = [storage](Async<QImage> &async) {
            async.setConcurrentCallData(&loadImage, storage->outputData);
        };
        const auto onImageLoadDone = [this, storage](const Async<QImage> &async) {
            if (!document() || !async.isResultAvailable())
                return;
            const QImage image = async.result();
            // FIXME use a self-implemented resource handler instead!
            document()->addResource(QTextDocument::ImageResource, m_loadingQueue.first(), QVariant(image));
            document()->markContentsDirty(0, document()->characterCount());
        };

        const auto onDone = [this] { m_loadingQueue.removeFirst(); };

        return For (iterator) >> Do {
            Group {
                storage,
                onGroupSetup(onSetup),
                If (downloadDataRecipe(storage)) >> Then {
                    AsyncTask<QImage>(onImageLoadSetup, onImageLoadDone, CallDoneIf::Success) || successItem
                },
                onGroupDone(onDone)
            }
        };
    }

    const Group m_recipe = recipe();
    QList<QUrl> m_loadingQueue;
    TaskTreeRunner m_loaderTaskTree;
};

class AxivionPerspective : public Perspective
{
public:
    AxivionPerspective() : Perspective("Axivion.Perspective", Tr::tr("Axivion")) {}
    void initPerspective();

    void handleShowIssues(const QString &kind);
    void handleShowFilterException(const QString &errorMessage);
    void handleShowErrorMessage(const QString &errorMessage);
    void reinitDashboardList(const QString &preferredProject);
    void resetDashboard();
    bool handleContextMenu(const QString &issue, const ItemViewEvent &e);
    void setIssueDetailsHtml(const QString &html) { m_issueDetails->setHtml(html); }
    void handleAnchorClicked(const QUrl &url);
    void updateToolbarButtons();

private:
    void openFilterHelp();

    IssuesWidget *m_issuesWidget = nullptr;
    LazyImageBrowser *m_issueDetails = nullptr;
    QAction *m_showFilterHelp = nullptr;
};

void AxivionPerspective::initPerspective()
{
    m_issuesWidget = new IssuesWidget;
    m_issuesWidget->setObjectName("AxivionIssuesWidget");
    m_issuesWidget->setWindowTitle(Tr::tr("Issues"));
    QPalette pal = m_issuesWidget->palette();
    pal.setColor(QPalette::Window, creatorColor(Theme::Color::BackgroundColorNormal));
    m_issuesWidget->setPalette(pal);

    m_issueDetails = new LazyImageBrowser;
    m_issueDetails->setFrameStyle(QFrame::NoFrame);
    m_issueDetails->setObjectName("AxivionIssuesDetails");
    m_issueDetails->setWindowTitle(Tr::tr("Issue Details"));
    const QString text = Tr::tr(
                "Search for issues inside the Axivion dashboard or request issue details for "
                "Axivion inline annotations to see them here.");
    m_issueDetails->setText("<p style='text-align:center'>" + text + "</p>");
    m_issueDetails->setOpenLinks(false);
    connect(m_issueDetails, &QTextBrowser::anchorClicked,
            this, &AxivionPerspective::handleAnchorClicked);

    auto reloadDataAct = new QAction(this);
    reloadDataAct->setIcon(Utils::Icons::RELOAD_TOOLBAR.icon());
    reloadDataAct->setToolTip(Tr::tr("Reload"));
    connect(reloadDataAct, &QAction::triggered, this, [this] {
        switchActiveDashboardId(activeDashboardId()); // reset cached data
        reinitDashboardList({});
    });

    auto showIssuesAct = new QAction(this);
    showIssuesAct->setIcon(MARKER_ICON.icon());
    showIssuesAct->setToolTip(Tr::tr("Show Issues in Editor"));
    showIssuesAct->setCheckable(true);
    showIssuesAct->setChecked(true);
    connect(showIssuesAct, &QAction::toggled,
            this, [](bool checked) {  enableInlineIssues(checked); });
    auto toggleIssuesAct = new QAction(this);
    toggleIssuesAct->setIcon(Utils::Icons::WARNING_TOOLBAR.icon());
    toggleIssuesAct->setToolTip(Tr::tr("Show Issue Annotations Inline"));
    toggleIssuesAct->setCheckable(true);
    toggleIssuesAct->setChecked(true);
    connect(toggleIssuesAct, &QAction::toggled, this, [](bool checked) {
        if (checked)
            TextEditor::TextDocument::showMarksAnnotation("AxivionTextMark");
        else
            TextEditor::TextDocument::temporaryHideMarksAnnotation("AxivionTextMark");
    });

    m_showFilterHelp = new QAction(this);
    m_showFilterHelp->setIcon(Utils::Icons::INFO_TOOLBAR.icon());
    m_showFilterHelp->setToolTip(Tr::tr("Show Online Filter Help"));
    m_showFilterHelp->setEnabled(false);
    connect(m_showFilterHelp, &QAction::triggered, this, &AxivionPerspective::openFilterHelp);

    addToolBarAction(reloadDataAct);
    addToolbarSeparator();
    addToolBarAction(showIssuesAct);
    addToolBarAction(toggleIssuesAct);
    addToolbarSeparator();
    addToolBarAction(m_showFilterHelp); // FIXME move to IssuesWidget when named filters are added

    addWindow(m_issuesWidget, Perspective::SplitVertical, nullptr);
    addWindow(m_issueDetails, Perspective::AddToTab, nullptr, true, Qt::RightDockWidgetArea);

    ActionContainer *menu = ActionManager::actionContainer(Debugger::Constants::M_DEBUG_ANALYZER);
    QAction *action = new QAction(Tr::tr("Axivion"), this);
    menu->addAction(ActionManager::registerAction(action, "Axivion.Perspective"),
                    Debugger::Constants::G_ANALYZER_TOOLS);
    connect(action, &QAction::triggered,
            this, &Perspective::select);
}

void AxivionPerspective::handleShowIssues(const QString &kind)
{
    m_issuesWidget->updateUi(kind);
}

void AxivionPerspective::handleShowFilterException(const QString &errorMessage)
{
    m_issuesWidget->showOverlay(errorMessage, IssuesWidget::ErrorIcon);
}

void AxivionPerspective::handleShowErrorMessage(const QString &errorMessage)
{
    m_issuesWidget->showErrorMessage(errorMessage);
}

void AxivionPerspective::reinitDashboardList(const QString &preferredProject)
{
    m_issuesWidget->initDashboardList(preferredProject);
}

void AxivionPerspective::resetDashboard()
{
    m_issuesWidget->resetDashboard();
}

bool AxivionPerspective::handleContextMenu(const QString &issue, const ItemViewEvent &e)
{
    if (!currentDashboardInfo())
        return false;
    const std::optional<Dto::TableInfoDto> tableInfoOpt = m_issuesWidget->currentTableInfo();
    if (!tableInfoOpt)
        return false;
    const QString baseUri = tableInfoOpt->issueBaseViewUri.value_or(QString());
    if (baseUri.isEmpty())
        return false;

    QUrl dashboardUrl = resolveDashboardInfoUrl(baseUri);
    QUrl issueBaseUrl = dashboardUrl.resolved(issue);
    const IssueListSearch search = m_issuesWidget->searchFromUi();
    issueBaseUrl.setQuery(search.toUrlQuery(QueryMode::SimpleQuery));
    dashboardUrl.setQuery(search.toUrlQuery(QueryMode::FilterQuery));

    QMenu *menu = new QMenu;
    auto action = new QAction(Tr::tr("Open Issue in Dashboard"), menu);
    menu->addAction(action);
    QObject::connect(action, &QAction::triggered, menu, [issueBaseUrl] {
        QDesktopServices::openUrl(issueBaseUrl);
    });
    action = new QAction(Tr::tr("Open Table in Dashboard"), menu);
    QObject::connect(action, &QAction::triggered, menu, [dashboardUrl] {
        QDesktopServices::openUrl(dashboardUrl);
    });
    menu->addAction(action);
    action = new QAction(Tr::tr("Copy Dashboard Link to Clipboard"), menu);
    QObject::connect(action, &QAction::triggered, menu, [dashboardUrl] {
        if (auto clipboard = QGuiApplication::clipboard())
            clipboard->setText(dashboardUrl.toString());
    });
    menu->addAction(action);
    QObject::connect(menu, &QMenu::aboutToHide, menu, &QObject::deleteLater);
    menu->popup(e.globalPos());
    return true;
}

void AxivionPerspective::handleAnchorClicked(const QUrl &url)
{
    if (!url.scheme().isEmpty()) {
        const QString detail = Tr::tr("The activated link appears to be external.\n"
                                      "Do you want to open \"%1\" with its default application?")
                .arg(url.toString());
        const QMessageBox::StandardButton pressed
            = CheckableMessageBox::question(Tr::tr("Open External Links"),
                                            detail,
                                            Key("AxivionOpenExternalLinks"));
        if (pressed == QMessageBox::Yes)
            QDesktopServices::openUrl(url);
        return;
    }
    const QUrlQuery query(url);
    if (query.isEmpty())
        return;
    Link link;
    if (const QString path = query.queryItemValue("filename", QUrl::FullyDecoded); !path.isEmpty())
        link.targetFilePath = findFileForIssuePath(FilePath::fromUserInput(path));
    if (const QString line = query.queryItemValue("line"); !line.isEmpty())
        link.targetLine = line.toInt();
    // column entry is wrong - so, ignore it
    if (link.hasValidTarget() && link.targetFilePath.exists())
        EditorManager::openEditorAt(link);
}

void AxivionPerspective::updateToolbarButtons()
{
    const std::optional<Dto::ProjectInfoDto> pInfo = projectInfo();
    m_showFilterHelp->setEnabled(pInfo && pInfo->issueFilterHelp);
}

void AxivionPerspective::openFilterHelp()
{
    const std::optional<Dto::ProjectInfoDto> projInfo = projectInfo();
    if (projInfo && projInfo->issueFilterHelp)
        QDesktopServices::openUrl(resolveDashboardInfoUrl(*projInfo->issueFilterHelp));
}

static QPointer<AxivionPerspective> theAxivionPerspective;

void setupAxivionPerspective()
{
    QTC_ASSERT(!theAxivionPerspective, return);
    theAxivionPerspective = new AxivionPerspective();
    theAxivionPerspective->initPerspective();
}

void updateDashboard()
{
    QTC_ASSERT(theAxivionPerspective, return);
    theAxivionPerspective->handleShowIssues({});
}

void reinitDashboard(const QString &preferredProject)
{
    QTC_ASSERT(theAxivionPerspective, return);
    theAxivionPerspective->reinitDashboardList(preferredProject);
}

void resetDashboard()
{
    QTC_ASSERT(theAxivionPerspective, return);
    theAxivionPerspective->resetDashboard();
}

static bool issueListContextMenuEvent(const ItemViewEvent &ev)
{
    QTC_ASSERT(theAxivionPerspective, return false);
    const QModelIndexList selectedIndices = ev.selectedRows();
    const QModelIndex first = selectedIndices.isEmpty() ? QModelIndex() : selectedIndices.first();
    if (!first.isValid())
        return false;
    const QString issue = first.data().toString();
    return theAxivionPerspective->handleContextMenu(issue, ev);
}

void showFilterException(const QString &errorMessage)
{
    QTC_ASSERT(theAxivionPerspective, return);
    theAxivionPerspective->handleShowFilterException(errorMessage);
}

void showErrorMessage(const QString &errorMessage)
{
    QTC_ASSERT(theAxivionPerspective, return);
    theAxivionPerspective->handleShowErrorMessage(errorMessage);
}

void updateIssueDetails(const QString &html)
{
    QTC_ASSERT(theAxivionPerspective, return);
    theAxivionPerspective->setIssueDetailsHtml(html);
}

void updatePerspectiveToolbar()
{
    QTC_ASSERT(theAxivionPerspective, return);
    theAxivionPerspective->updateToolbarButtons();
}

} // Axivion::Internal
