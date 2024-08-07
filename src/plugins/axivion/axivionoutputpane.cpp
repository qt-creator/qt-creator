// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionoutputpane.h"

#include "axivionplugin.h"
#include "axivionsettings.h"
#include "axiviontr.h"
#include "dashboard/dto.h"
#include "issueheaderview.h"
#include "dynamiclistmodel.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/ioutputpane.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/algorithm.h>
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
#include <QFormLayout>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QToolButton>
#include <QUrlQuery>

#include <map>

using namespace Core;
using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace Axivion::Internal {

void showIssuesFromDashboard(const QString &kind); // impl at bottom

class DashboardWidget : public QScrollArea
{
public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    void updateUi();
    bool hasProject() const { return !m_project->text().isEmpty(); }
private:
    QLabel *m_project = nullptr;
    QLabel *m_loc = nullptr;
    QLabel *m_timestamp = nullptr;
    QGridLayout *m_gridLayout = nullptr;
};

DashboardWidget::DashboardWidget(QWidget *parent)
    : QScrollArea(parent)
{
    QWidget *widget = new QWidget(this);
    m_project = new QLabel(this);
    m_loc = new QLabel(this);
    m_timestamp = new QLabel(this);

    m_gridLayout = new QGridLayout;

    using namespace Layouting;
    Column {
        Form {
            Tr::tr("Project:"), m_project, br,
            Tr::tr("Lines of code:"), m_loc, br,
            Tr::tr("Analysis timestamp:"), m_timestamp
        },
        Space(10),
        Row { m_gridLayout, st },
        st
    }.attachTo(widget);

    setWidget(widget);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setWidgetResizable(true);
}

static QPixmap trendIcon(qint64 added, qint64 removed)
{
    static const QPixmap unchanged = Icons::NEXT.pixmap();
    static const QPixmap increased = Icon(
                { {":/utils/images/arrowup.png", Theme::IconsErrorColor} }).pixmap();
    static const QPixmap decreased = Icon(
                {  {":/utils/images/arrowdown.png", Theme::IconsRunColor} }).pixmap();
    if (added == removed)
        return unchanged;
    return added < removed ? decreased : increased;
}

static qint64 extract_value(const std::map<QString, Dto::Any> &map, const QString &key)
{
    const auto search = map.find(key);
    if (search == map.end())
        return 0;
    const Dto::Any &value = search->second;
    if (!value.isDouble())
        return 0;
    return static_cast<qint64>(value.getDouble());
}

void DashboardWidget::updateUi()
{
    m_project->setText({});
    m_loc->setText({});
    m_timestamp->setText({});
    QLayoutItem *child;
    while ((child = m_gridLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    std::optional<Dto::ProjectInfoDto> projectInfo = Internal::projectInfo();
    if (!projectInfo)
        return;
    const Dto::ProjectInfoDto &info = *projectInfo;
    m_project->setText(info.name);
    if (info.versions.empty())
        return;

    const Dto::AnalysisVersionDto &last = info.versions.back();
    setAnalysisVersion(last.date);
    if (last.linesOfCode.has_value())
        m_loc->setText(QString::number(last.linesOfCode.value()));
    const QDateTime timeStamp = QDateTime::fromString(last.date, Qt::ISODate);
    m_timestamp->setText(timeStamp.isValid() ? timeStamp.toString("yyyy-MM-dd HH:mm:ss t")
                                             : Tr::tr("unknown"));

    const std::vector<Dto::IssueKindInfoDto> &issueKinds = info.issueKinds;
    auto toolTip = [issueKinds](const QString &prefix){
        for (const Dto::IssueKindInfoDto &kind : issueKinds) {
            if (kind.prefix == prefix)
                return kind.nicePluralName;
        }
        return prefix;
    };
    auto linked = [](const QString &text, const QString &href, bool link) {
        return link ? QString("<a href='%1'>%2</a>").arg(href).arg(text)
                    : text;
    };
    auto addValuesWidgets = [this, &toolTip, &linked](const QString &issueKind, qint64 total,
            qint64 added, qint64 removed, int row, bool link) {
        const QString currentToolTip = toolTip(issueKind);
        QLabel *label = new QLabel(issueKind, this);
        label->setToolTip(currentToolTip);
        m_gridLayout->addWidget(label, row, 0);
        label = new QLabel(linked(QString::number(total), issueKind, link), this);
        label->setToolTip(currentToolTip);
        label->setAlignment(Qt::AlignRight);
        if (link) {
            connect(label, &QLabel::linkActivated, this, [](const QString &issueKind) {
                showIssuesFromDashboard(issueKind);
            });
        }
        m_gridLayout->addWidget(label, row, 1);
        label = new QLabel(this);
        label->setPixmap(trendIcon(added, removed));
        label->setToolTip(currentToolTip);
        m_gridLayout->addWidget(label, row, 2);
        label = new QLabel('+' + QString::number(added));
        label->setAlignment(Qt::AlignRight);
        label->setToolTip(currentToolTip);
        m_gridLayout->addWidget(label, row, 3);
        label = new QLabel("/");
        label->setToolTip(currentToolTip);
        m_gridLayout->addWidget(label, row, 4);
        label = new QLabel('-' + QString::number(removed));
        label->setAlignment(Qt::AlignRight);
        label->setToolTip(currentToolTip);
        m_gridLayout->addWidget(label, row, 5);
    };
    qint64 allTotal = 0;
    qint64 allAdded = 0;
    qint64 allRemoved = 0;
    qint64 row = 0;
    // This code is overly complex because of a heedlessness in the
    // Axivion Dashboard API definition. Other Axivion IDE plugins do
    // not use the issue counts, thus the QtCreator Axivion Plugin
    // is going to stop using them, too.
    if (last.issueCounts.isMap()) {
        for (const Dto::Any::MapEntry &issueCount : last.issueCounts.getMap()) {
            if (issueCount.second.isMap()) {
                const Dto::Any::Map &counts = issueCount.second.getMap();
                qint64 total = extract_value(counts, QStringLiteral("Total"));
                allTotal += total;
                qint64 added = extract_value(counts, QStringLiteral("Added"));
                allAdded += added;
                qint64 removed = extract_value(counts, QStringLiteral("Removed"));
                allRemoved += removed;
                addValuesWidgets(issueCount.first, total, added, removed, row, true);
                ++row;
            }
        }
    }
    addValuesWidgets(Tr::tr("Total:"), allTotal, allAdded, allRemoved, row, false);
}

struct LinkWithColumns
{
    Link link;
    QList<int> columns;
};

static bool issueListContextMenuEvent(const ItemViewEvent &ev); // impl at bottom

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
                Project *project = ProjectManager::startupProject();
                FilePath baseDir = project ? project->projectDirectory() : FilePath{};
                link.targetFilePath = findFileForIssuePath(link.targetFilePath);
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

    const std::optional<Dto::TableInfoDto> currentTableInfo() const { return m_currentTableInfo; }
    IssueListSearch searchFromUi() const;
private:
    void updateTable();
    void addIssues(const Dto::IssueTableDto &dto, int startRow);
    void onSearchParameterChanged();
    void updateBasicProjectInfo(const std::optional<Dto::ProjectInfoDto> &info);
    void setFiltersEnabled(bool enabled);
    void fetchTable();
    void fetchIssues(const IssueListSearch &search);
    void onFetchRequested(int startRow, int limit);
    void showNoDataOverlay();
    void hideNoDataOverlay();

    QString m_currentPrefix;
    QString m_currentProject;
    std::optional<Dto::TableInfoDto> m_currentTableInfo;
    QHBoxLayout *m_typesLayout = nullptr;
    QButtonGroup *m_typesButtonGroup = nullptr;
    QPushButton *m_addedFilter = nullptr;
    QPushButton *m_removedFilter = nullptr;
    QComboBox *m_ownerFilter = nullptr;
    QComboBox *m_versionStart = nullptr;
    QComboBox *m_versionEnd = nullptr;
    Guard m_signalBlocker;
    QLineEdit *m_pathGlobFilter = nullptr; // FancyLineEdit instead?
    QLabel *m_totalRows = nullptr;
    BaseTreeView *m_issuesView = nullptr;
    IssueHeaderView *m_headerView = nullptr;
    DynamicListModel *m_issuesModel = nullptr;
    int m_totalRowCount = 0;
    QStringList m_userNames;
    QStringList m_versionDates;
    TaskTreeRunner m_taskTreeRunner;
    OverlayWidget *m_noDataOverlay = nullptr;
};

IssuesWidget::IssuesWidget(QWidget *parent)
    : QScrollArea(parent)
{
    QWidget *widget = new QWidget(this);
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
        onSearchParameterChanged();
    });

    m_versionEnd = new QComboBox(this);
    m_versionEnd->setMinimumContentsLength(25);
    connect(m_versionEnd, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (m_signalBlocker.isLocked())
            return;
        QTC_ASSERT(index > -1 && index < m_versionDates.size(), return);
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
    m_issuesView->setHeader(m_headerView);
    m_issuesView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_issuesView->enableColumnHiding();
    m_issuesModel = new DynamicListModel(this);
    m_issuesView->setModel(m_issuesModel);
    connect(m_issuesModel, &DynamicListModel::fetchRequested, this, &IssuesWidget::onFetchRequested);
    m_totalRows = new QLabel(Tr::tr("Total rows:"), this);

    using namespace Layouting;
    Column {
        Row { m_typesLayout, st, m_versionStart, m_versionEnd, st },
        Row { m_addedFilter, m_removedFilter, Space(1), m_ownerFilter, m_pathGlobFilter },
        m_issuesView,
        Row { st, m_totalRows }
    }.attachTo(widget);

    setWidget(widget);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setWidgetResizable(true);
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
            hiddenColumns << column.key;
        IssueHeaderView::ColumnInfo info;
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
            Link link{ FilePath::fromUserInput(it->second.getString()) };
            columns.append(findColumn(it->first));
            it = issueRow.find(line);
            if (it != end) {
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
        m_totalRowCount = dto.totalRowCount.value();
        m_issuesModel->setExpectedRowCount(m_totalRowCount);
        m_totalRows->setText(Tr::tr("Total rows:") + ' ' + QString::number(m_totalRowCount));
    }
    if (dto.totalAddedCount.has_value())
        m_addedFilter->setText(QString::number(dto.totalAddedCount.value()));
    if (dto.totalRemovedCount.has_value())
        m_removedFilter->setText(QString::number(dto.totalRemovedCount.value()));

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
        showNoDataOverlay();
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
        m_issuesModel->clear();
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

    m_userNames.clear();
    m_ownerFilter->clear();
    QStringList userDisplayNames;
    for (const Dto::UserRefDto &user : info->users) {
        userDisplayNames.append(user.displayName);
        m_userNames.append(user.name);
    }
    m_signalBlocker.lock();
    m_ownerFilter->addItems(userDisplayNames);
    m_signalBlocker.unlock();

    m_versionDates.clear();
    m_versionStart->clear();
    m_versionEnd->clear();
    QStringList versionLabels;
    const std::vector<Dto::AnalysisVersionDto> &versions = info->versions;
    for (auto it = versions.crbegin(); it != versions.crend(); ++it) {
        const Dto::AnalysisVersionDto &version = *it;
        versionLabels.append(version.label.value_or(version.name));
        m_versionDates.append(version.date);
    }
    m_signalBlocker.lock();
    m_versionStart->addItems(versionLabels);
    m_versionEnd->addItems(versionLabels);
    m_versionStart->setCurrentIndex(m_versionDates.count() - 1);
    m_signalBlocker.unlock();
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
    search.kind = m_currentPrefix; // not really ui.. but anyhow
    search.owner = m_userNames.at(m_ownerFilter->currentIndex());
    search.filter_path = m_pathGlobFilter->text();
    search.versionStart = m_versionDates.at(m_versionStart->currentIndex());
    search.versionEnd = m_versionDates.at(m_versionEnd->currentIndex());
    // different approach: checked means disabling in webview, checked here means explicitly request
    // the checked one, having both checked is impossible (having none checked means fetch both)
    // reason for different approach: currently poor reflected inside the ui (TODO)
    if (m_addedFilter->isChecked())
        search.state = "added";
    else if (m_removedFilter->isChecked())
        search.state = "removed";

    QTC_ASSERT(m_currentTableInfo, return search);
    QString sort;
    const QList<QPair<int, Qt::SortOrder>> currentSortColumns = m_headerView->currentSortColumns();
    for (const auto &pair : currentSortColumns) {
        QTC_ASSERT((ulong)pair.first < m_currentTableInfo->columns.size(), return search);
        if (!sort.isEmpty())
            sort.append(',');
        sort.append(m_currentTableInfo->columns.at(pair.first).key
                    + (pair.second == Qt::AscendingOrder ? " asc" : " desc"));
    }
    search.sort = sort;
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
    hideNoDataOverlay();
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

void IssuesWidget::showNoDataOverlay()
{
    if (!m_noDataOverlay) {
        QTC_ASSERT(m_issuesView, return);
        m_noDataOverlay = new OverlayWidget(this);
        m_noDataOverlay->setPaintFunction([](QWidget *that, QPainter &p, QPaintEvent *) {
                static const QIcon icon = Icon({{":/axivion/images/nodata.png",
                                                 Theme::IconsDisabledColor}},
                                               Utils::Icon::Tint).icon();
                QRect iconRect(0, 0, 32, 32);
                iconRect.moveCenter(that->rect().center());
                icon.paint(&p, iconRect);
                p.save();
                p.setPen(Utils::creatorColor(Theme::TextColorDisabled));
                p.drawText(iconRect.bottomRight() + QPoint{10, p.fontMetrics().height() / 2 - 16},
                           Tr::tr("No Data"));
                p.restore();
            });
        m_noDataOverlay->attachToWidget(m_issuesView);
    }
    m_noDataOverlay->show();
}

void IssuesWidget::hideNoDataOverlay()
{
    if (m_noDataOverlay)
        m_noDataOverlay->hide();
}

class AxivionOutputPane final : public IOutputPane
{
public:
    explicit AxivionOutputPane(QObject *parent)
        : IOutputPane(parent)
    {
        setId("Axivion");
        setDisplayName(Tr::tr("Axivion"));
        setPriorityInStatusBar(-50);

        m_outputWidget = new QStackedWidget;
        DashboardWidget *dashboardWidget = new DashboardWidget(m_outputWidget);
        m_outputWidget->addWidget(dashboardWidget);
        IssuesWidget *issuesWidget = new IssuesWidget(m_outputWidget);
        m_outputWidget->addWidget(issuesWidget);

        QPalette pal = m_outputWidget->palette();
        pal.setColor(QPalette::Window, creatorColor(Theme::Color::BackgroundColorNormal));
        m_outputWidget->setPalette(pal);

        m_showDashboard = new QToolButton(m_outputWidget);
        m_showDashboard->setIcon(Icons::HOME_TOOLBAR.icon());
        m_showDashboard->setToolTip(Tr::tr("Show dashboard"));
        m_showDashboard->setCheckable(true);
        m_showDashboard->setChecked(true);
        connect(m_showDashboard, &QToolButton::clicked, this, [this] {
            QTC_ASSERT(m_outputWidget, return);
            m_outputWidget->setCurrentIndex(0);
        });

        m_showIssues = new QToolButton(m_outputWidget);
        m_showIssues->setIcon(Icons::ZOOM_TOOLBAR.icon());
        m_showIssues->setToolTip(Tr::tr("Search for issues"));
        m_showIssues->setCheckable(true);
        connect(m_showIssues, &QToolButton::clicked, this, [this] { handleShowIssues({}); });
        auto *butonGroup = new QButtonGroup(this);
        butonGroup->addButton(m_showDashboard);
        butonGroup->addButton(m_showIssues);
        butonGroup->setExclusive(true);

        connect(m_outputWidget, &QStackedWidget::currentChanged, this, [this](int idx) {
            m_showDashboard->setChecked(idx == 0);
            m_showIssues->setChecked(idx == 1);
        });

        m_toggleIssues = new QToolButton(m_outputWidget);
        m_toggleIssues->setIcon(Utils::Icons::WARNING_TOOLBAR.icon());
        m_toggleIssues->setToolTip(Tr::tr("Show issue markers inline"));
        m_toggleIssues->setCheckable(true);
        m_toggleIssues->setChecked(settings().highlightMarks());
        connect(m_toggleIssues, &QToolButton::toggled, this, [](bool checked) {
            settings().highlightMarks.setValue(checked);
        });
        connect(&settings().highlightMarks, &BaseAspect::changed, this, [this] {
            m_toggleIssues->setChecked(settings().highlightMarks());
        });
    }

    ~AxivionOutputPane()
    {
        if (!m_outputWidget->parent())
            delete m_outputWidget;
    }

    QWidget *outputWidget(QWidget *parent) final
    {
        if (m_outputWidget)
            m_outputWidget->setParent(parent);
        else
            QTC_CHECK(false);
        return m_outputWidget;
    }

    QList<QWidget *> toolBarWidgets() const final
    {
        return {m_showDashboard, m_showIssues, m_toggleIssues};
    }

    void clearContents() final {}
    void setFocus() final {}
    bool hasFocus() const final { return false; }
    bool canFocus() const final { return true; }
    bool canNavigate() const final { return true; }
    bool canNext() const final { return false; }
    bool canPrevious() const final { return false; }
    void goToNext() final {}
    void goToPrev() final {}

    void handleShowIssues(const QString &kind)
    {
        QTC_ASSERT(m_outputWidget, return);
        m_outputWidget->setCurrentIndex(1);
        if (auto issues = static_cast<IssuesWidget *>(m_outputWidget->widget(1)))
            issues->updateUi(kind);
    }

    void updateDashboard()
    {
        if (auto dashboard = static_cast<DashboardWidget *>(m_outputWidget->widget(0))) {
            dashboard->updateUi();
            m_outputWidget->setCurrentIndex(0);
            if (dashboard->hasProject())
                flash();
        }
    }

    bool handleContextMenu(const QString &issue, const ItemViewEvent &e)
    {
        auto issues = static_cast<IssuesWidget *>(m_outputWidget->widget(1));
        std::optional<Dto::TableInfoDto> tableInfoOpt = issues ? issues->currentTableInfo()
                                                               : std::nullopt;
        if (!tableInfoOpt)
            return false;
        const QString baseUri = tableInfoOpt->issueBaseViewUri.value_or(QString());
        if (baseUri.isEmpty())
            return false;
        auto info = currentDashboardInfo();
        if (!info)
            return false;

        QUrl issueBaseUrl = info->source.resolved(baseUri).resolved(issue);
        QUrl dashboardUrl = info->source.resolved(baseUri);
        const IssueListSearch search = issues->searchFromUi();
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

private:
    QStackedWidget *m_outputWidget = nullptr;
    QToolButton *m_showDashboard = nullptr;
    QToolButton *m_showIssues = nullptr;
    QToolButton *m_toggleIssues = nullptr;
};


static QPointer<AxivionOutputPane> theAxivionOutputPane;

void setupAxivionOutputPane(QObject *guard)
{
    theAxivionOutputPane = new AxivionOutputPane(guard);
}

void updateDashboard()
{
    QTC_ASSERT(theAxivionOutputPane, return);
    theAxivionOutputPane->updateDashboard();
}

static bool issueListContextMenuEvent(const ItemViewEvent &ev)
{
    QTC_ASSERT(theAxivionOutputPane, return false);
    const QModelIndexList selectedIndices = ev.selectedRows();
    const QModelIndex first = selectedIndices.isEmpty() ? QModelIndex() : selectedIndices.first();
    if (!first.isValid())
        return false;
    const QString issue = first.data().toString();
    return theAxivionOutputPane->handleContextMenu(issue, ev);
}

void showIssuesFromDashboard(const QString &kind)
{
    QTC_ASSERT(theAxivionOutputPane, return);
    theAxivionOutputPane->handleShowIssues(kind);
}

} // Axivion::Internal
