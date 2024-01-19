// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionoutputpane.h"

#include "axivionplugin.h"
#include "axiviontr.h"
#include "dashboard/dto.h"

#include <utils/qtcassert.h>
#include <utils/treemodel.h>
#include <utils/basetreeview.h>
#include <utils/utilsicons.h>

#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QToolButton>

#include <map>
#include <memory>

namespace Axivion::Internal {

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
    QVBoxLayout *layout = new QVBoxLayout(widget);
    QFormLayout *projectLayout = new QFormLayout;
    m_project = new QLabel(this);
    projectLayout->addRow(Tr::tr("Project:"), m_project);
    m_loc = new QLabel(this);
    projectLayout->addRow(Tr::tr("Lines of code:"), m_loc);
    m_timestamp = new QLabel(this);
    projectLayout->addRow(Tr::tr("Analysis timestamp:"), m_timestamp);
    layout->addLayout(projectLayout);
    layout->addSpacing(10);
    auto row = new QHBoxLayout;
    m_gridLayout = new QGridLayout;
    row->addLayout(m_gridLayout);
    row->addStretch(1);
    layout->addLayout(row);
    layout->addStretch(1);
    setWidget(widget);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setWidgetResizable(true);
}

static QPixmap trendIcon(qint64 added, qint64 removed)
{
    static const QPixmap unchanged = Utils::Icons::NEXT.pixmap();
    static const QPixmap increased = Utils::Icon(
                { {":/utils/images/arrowup.png", Utils::Theme::IconsErrorColor} }).pixmap();
    static const QPixmap decreased = Utils::Icon(
                {  {":/utils/images/arrowdown.png", Utils::Theme::IconsRunColor} }).pixmap();
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
    auto addValuesWidgets = [this, &toolTip](const QString &issueKind, qint64 total, qint64 added, qint64 removed, int row) {
        const QString currentToolTip = toolTip(issueKind);
        QLabel *label = new QLabel(issueKind, this);
        label->setToolTip(currentToolTip);
        m_gridLayout->addWidget(label, row, 0);
        label = new QLabel(QString::number(total), this);
        label->setToolTip(currentToolTip);
        label->setAlignment(Qt::AlignRight);
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
                addValuesWidgets(issueCount.first, total, added, removed, row);
                ++row;
            }
        }
    }
    addValuesWidgets(Tr::tr("Total:"), allTotal, allAdded, allRemoved, row);
}

class IssuesWidget : public QScrollArea
{
public:
    explicit IssuesWidget(QWidget *parent = nullptr);
    void updateUi();
    void setTableDto(const Dto::TableInfoDto &dto);
    void addIssues(const Dto::IssueTableDto &dto);

private:
    void updateTableView();

    QString m_currentPrefix;
    std::optional<Dto::TableInfoDto> m_currentTableInfo;
    QHBoxLayout *m_typesLayout = nullptr;
    QHBoxLayout *m_filtersLayout = nullptr;
    QPushButton *m_addedFilter = nullptr;
    QPushButton *m_removedFilter = nullptr;
    QComboBox *m_ownerFilter = nullptr;
    QComboBox *m_versionStart = nullptr;
    QComboBox *m_versionEnd = nullptr;
    QLineEdit *m_pathGlobFilter = nullptr; // FancyLineEdit instead?
    Utils::BaseTreeView *m_issuesView = nullptr;
    Utils::TreeModel<> *m_issuesModel = nullptr;
};

IssuesWidget::IssuesWidget(QWidget *parent)
    : QScrollArea(parent)
{
    QWidget *widget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(widget);
    // row with issue types (-> depending on choice, tables below change)
    //  and a selectable range (start version, end version)
    // row with added/removed and some filters (assignee, path glob, (named filter))
    // table, columns depend on chosen issue type
    QHBoxLayout *top = new QHBoxLayout;
    layout->addLayout(top);
    m_typesLayout = new QHBoxLayout;
    top->addLayout(m_typesLayout);
    top->addStretch(1);
    m_versionStart = new QComboBox(this);
    m_versionStart->setMinimumContentsLength(25);
    top->addWidget(m_versionStart);
    m_versionEnd = new QComboBox(this);
    m_versionEnd->setMinimumContentsLength(25);
    top->addWidget(m_versionEnd);
    top->addStretch(1);
    m_filtersLayout = new QHBoxLayout;
    m_addedFilter = new QPushButton(this);
    m_addedFilter->setIcon(trendIcon(1, 0));
    m_addedFilter->setText("0");
    m_filtersLayout->addWidget(m_addedFilter);
    m_removedFilter = new QPushButton(this);
    m_removedFilter->setIcon(trendIcon(0, 1));
    m_removedFilter->setText("0");
    m_filtersLayout->addWidget(m_removedFilter);
    m_filtersLayout->addSpacing(1);
    m_ownerFilter = new QComboBox(this);
    m_ownerFilter->setToolTip(Tr::tr("Owner"));
    m_ownerFilter->setMinimumContentsLength(25);
    m_filtersLayout->addWidget(m_ownerFilter);
    m_pathGlobFilter = new QLineEdit(this);
    m_pathGlobFilter->setPlaceholderText(Tr::tr("Path globbing"));
    m_filtersLayout->addWidget(m_pathGlobFilter);
    layout->addLayout(m_filtersLayout);
    m_issuesView = new Utils::BaseTreeView(this);
    m_issuesView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_issuesView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_issuesView->enableColumnHiding();
    m_issuesModel = new Utils::TreeModel;
    m_issuesView->setModel(m_issuesModel);
    layout->addWidget(m_issuesView);
    layout->addStretch(1);
    setWidget(widget);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setWidgetResizable(true);
}

void IssuesWidget::updateUi()
{
    m_filtersLayout->setEnabled(false);
    // TODO extract parts of it and do them only when necessary
    QLayoutItem *child;
    while ((child = m_typesLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    std::optional<Dto::ProjectInfoDto> projectInfo = Internal::projectInfo();
    if (!projectInfo)
        return;
    const Dto::ProjectInfoDto &info = *projectInfo;
    if (info.versions.empty()) // add some warning/information?
        return;

    // for now just a start..
    const Dto::AnalysisVersionDto &last = info.versions.back();

    const std::vector<Dto::IssueKindInfoDto> &issueKinds = info.issueKinds;
    for (const Dto::IssueKindInfoDto &kind : issueKinds) {
        auto button = new QToolButton(this);
        button->setIcon(iconForIssue(kind.prefix));
        button->setToolTip(kind.nicePluralName);
        connect(button, &QToolButton::clicked, this, [this, prefix = kind.prefix]{
            m_currentPrefix = prefix;
            updateTableView();
        });
        m_typesLayout->addWidget(button);
    }
    // TODO versions range...

    // TODO fill owners
    m_filtersLayout->setEnabled(true);
    if (info.issueKinds.size())
        m_currentPrefix = info.issueKinds.front().prefix;
    updateTableView();
}


void IssuesWidget::setTableDto(const Dto::TableInfoDto &dto)
{
    m_currentTableInfo.emplace(dto);

    // update issues table layout - for now just simple approach
    Utils::TreeModel<> *issuesModel = new Utils::TreeModel;
    QStringList columnHeaders;
    QStringList hiddenColumns;
    for (const Dto::ColumnInfoDto &column : dto.columns) {
        columnHeaders << column.key;
        if (!column.showByDefault)
            hiddenColumns << column.key;
    }

    issuesModel->setHeader(columnHeaders);

    auto oldModel = m_issuesModel;
    m_issuesModel = issuesModel;
    m_issuesView->setModel(issuesModel);
    delete oldModel;
    int counter = 0;
    for (const QString &header : std::as_const(columnHeaders))
        m_issuesView->setColumnHidden(counter++, hiddenColumns.contains(header));

    // first time lookup... should we cache and maybe represent old data?
    IssueListSearch search;
    search.kind = m_currentPrefix;
    fetchIssues(search);
}

void IssuesWidget::addIssues(const Dto::IssueTableDto &dto)
{
    QTC_ASSERT(m_currentTableInfo.has_value(), return);
    // handle added/removed/total ?

    const std::vector<Dto::ColumnInfoDto> tableColumns = m_currentTableInfo->columns;
    // const std::vector<Dto::ColumnInfoDto> columns = dto.columns.value();
    const std::vector<std::map<QString, Dto::Any>> rows = dto.rows;
    for (auto row : rows) {
        QStringList data;
        for (auto column : tableColumns) {
            auto it = row.find(column.key);
            if (it != row.end()) {
                if (it->second.isString())
                    data << it->second.getString();
                else if (it->second.isDouble())
                    data << QString::number(it->second.getDouble());
                else if (it->second.isBool())
                    data << QString("%1").arg(it->second.getBool());
                else
                    data << "not yet";
            }
        }
        Utils::StaticTreeItem *it = new Utils::StaticTreeItem(data);
        m_issuesModel->rootItem()->appendChild(it);
    }
}

void IssuesWidget::updateTableView()
{
    QTC_ASSERT(!m_currentPrefix.isEmpty(), return);
    // fetch table dto and apply, on done fetch first data for the selected issues
    fetchIssueTableLayout(m_currentPrefix);
}

AxivionOutputPane::AxivionOutputPane(QObject *parent)
    : Core::IOutputPane(parent)
{
    setId("Axivion");
    setDisplayName(Tr::tr("Axivion"));
    setPriorityInStatusBar(-50);

    m_outputWidget = new QStackedWidget;
    DashboardWidget *dashboardWidget = new DashboardWidget(m_outputWidget);
    m_outputWidget->addWidget(dashboardWidget);
    IssuesWidget *issuesWidget = new IssuesWidget(m_outputWidget);
    m_outputWidget->addWidget(issuesWidget);
    QTextBrowser *browser = new QTextBrowser(m_outputWidget);
    m_outputWidget->addWidget(browser);
}

AxivionOutputPane::~AxivionOutputPane()
{
    if (!m_outputWidget->parent())
        delete m_outputWidget;
}

QWidget *AxivionOutputPane::outputWidget(QWidget *parent)
{
    if (m_outputWidget)
        m_outputWidget->setParent(parent);
    else
        QTC_CHECK(false);
    return m_outputWidget;
}

QList<QWidget *> AxivionOutputPane::toolBarWidgets() const
{
    QList<QWidget *> buttons;
    auto showDashboard = new QToolButton(m_outputWidget);
    showDashboard->setIcon(Utils::Icons::HOME_TOOLBAR.icon());
    showDashboard->setToolTip(Tr::tr("Show dashboard"));
    connect(showDashboard, &QToolButton::clicked, this, [this]{
        QTC_ASSERT(m_outputWidget, return);
        m_outputWidget->setCurrentIndex(0);
    });
    buttons.append(showDashboard);
    auto showIssues = new QToolButton(m_outputWidget);
    showIssues->setIcon(Utils::Icons::ZOOM_TOOLBAR.icon());
    showIssues->setToolTip(Tr::tr("Search for issues"));
    connect(showIssues, &QToolButton::clicked, this, [this]{
        QTC_ASSERT(m_outputWidget, return);
        m_outputWidget->setCurrentIndex(1);
        if (auto issues = static_cast<IssuesWidget *>(m_outputWidget->widget(1)))
            issues->updateUi();
    });
    buttons.append(showIssues);
    return buttons;
}

void AxivionOutputPane::clearContents()
{
}

void AxivionOutputPane::setFocus()
{
}

bool AxivionOutputPane::hasFocus() const
{
    return false;
}

bool AxivionOutputPane::canFocus() const
{
    return true;
}

bool AxivionOutputPane::canNavigate() const
{
    return true;
}

bool AxivionOutputPane::canNext() const
{
    return false;
}

bool AxivionOutputPane::canPrevious() const
{
    return false;
}

void AxivionOutputPane::goToNext()
{
}

void AxivionOutputPane::goToPrev()
{
}

void AxivionOutputPane::updateDashboard()
{
    if (auto dashboard = static_cast<DashboardWidget *>(m_outputWidget->widget(0))) {
        dashboard->updateUi();
        m_outputWidget->setCurrentIndex(0);
        if (dashboard->hasProject())
            flash();
    }
}

void AxivionOutputPane::setTableDto(const Dto::TableInfoDto &dto)
{
    if (auto issues = static_cast<IssuesWidget *>(m_outputWidget->widget(1)))
        issues->setTableDto(dto);
}

void AxivionOutputPane::addIssues(const Dto::IssueTableDto &dto)
{
    if (auto issues = static_cast<IssuesWidget *>(m_outputWidget->widget(1)))
        issues->addIssues(dto);
}

void AxivionOutputPane::updateAndShowRule(const QString &ruleHtml)
{
    if (auto browser = static_cast<QTextBrowser *>(m_outputWidget->widget(2))) {
        browser->setText(ruleHtml);
        if (!ruleHtml.isEmpty()) {
            m_outputWidget->setCurrentIndex(2);
            popup(Core::IOutputPane::NoModeSwitch);
        }
    }
}

} // Axivion::Internal
