// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionoutputpane.h"

#include "axivionplugin.h"
#include "axiviontr.h"
#include "dashboard/dto.h"

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QFormLayout>
#include <QGridLayout>
#include <QLabel>
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
    std::shared_ptr<const DashboardClient::ProjectInfo> projectInfo = AxivionPlugin::projectInfo();
    if (!projectInfo)
        return;
    const Dto::ProjectInfoDto &info = projectInfo->data;
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
                qint64 total = extract_value(counts, QStringLiteral(u"Total"));
                allTotal += total;
                qint64 added = extract_value(counts, QStringLiteral(u"Added"));
                allAdded += added;
                qint64 removed = extract_value(counts, QStringLiteral(u"Removed"));
                allRemoved += removed;
                addValuesWidgets(issueCount.first, total, added, removed, row);
                ++row;
            }
        }
    }
    addValuesWidgets(Tr::tr("Total:"), allTotal, allAdded, allRemoved, row);
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

void AxivionOutputPane::updateAndShowRule(const QString &ruleHtml)
{
    if (auto browser = static_cast<QTextBrowser *>(m_outputWidget->widget(1))) {
        browser->setText(ruleHtml);
        if (!ruleHtml.isEmpty()) {
            m_outputWidget->setCurrentIndex(1);
            popup(Core::IOutputPane::NoModeSwitch);
        }
    }
}

} // Axivion::Internal
