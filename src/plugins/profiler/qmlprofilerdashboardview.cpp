// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerdashboardview.h"

#include "profilertr.h"
#include "qmlprofilerdashboardstats.h"
#include "qmlprofilerfindingsmodel.h"
#include "qmlprofilermodelmanager.h"

#include <utils/icon.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/qtdesignwidgets.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QEnterEvent>
#include <QFrame>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

using namespace Utils;
using namespace Utils::StyleHelper;

namespace Profiler::Internal {

enum Size {
    Small,
    Large,
};

constexpr TextFormat titleTf {
    .themeColor = Theme::Token_Text_Default,
    .uiElement = UiElementH4,
    .drawTextFlags = Qt::AlignCenter,
};

constexpr TextFormat titleSmallTf {
    .themeColor = titleTf.themeColor,
    .uiElement = UiElementH6,
    .drawTextFlags = titleTf.drawTextFlags,
};

constexpr TextFormat textTf {
    .themeColor = Theme::Token_Text_Muted,
    .uiElement = UiElementBody1,
    .drawTextFlags = Qt::AlignCenter,
};

constexpr TextFormat textSmallTf {
    .themeColor = textTf.themeColor,
    .uiElement = UiElementBody2,
    .drawTextFlags = textTf.drawTextFlags,
};

constexpr TextFormat textFrameAnalysisTf {
    .themeColor = textTf.themeColor,
    .uiElement = textTf.uiElement,
    .drawTextFlags = Qt::AlignLeft,
};

constexpr TextFormat scoreWidgetTf {
    .themeColor = Theme::Token_Text_Default,
    .uiElement = UiElementH5,
    .drawTextFlags = Qt::AlignVCenter,
};

class ScoreWidget : public QWidget
{
public:
    enum Score {
        Excellent,
        Good,
        Alerting,
        Poor,
    };

    ScoreWidget(QWidget *parent = nullptr);

    void setScore(Score score);

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    const int m_iconSize = 24;
    QString m_text;
    Theme::Color m_color = Theme::Token_Notification_Success_Default;
    Theme::Color m_bgColor = Theme::Token_Notification_Success_Subtle;
    QIcon m_icon;
};

ScoreWidget::ScoreWidget(QWidget *parent)
    : QWidget(parent)
{
    setScore(Excellent);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
}

void ScoreWidget::paintEvent([[maybe_unused]] QPaintEvent *event)
{
    QPainter p(this);
    drawCardBg(&p, rect(), creatorColor(m_bgColor));
    const QRect iconR(SpacingTokens::PaddingHS, SpacingTokens::PaddingVXs, m_iconSize, m_iconSize);
    m_icon.paint(&p, iconR);
    const QRect textR(iconR.right() + SpacingTokens::GapHXs, 0, 1000, height());
    p.setFont(scoreWidgetTf.font());
    p.setPen(creatorColor(m_color));
    p.drawText(textR, scoreWidgetTf.drawTextFlags, m_text);
}

static InfoLabelType infoType(ScoreWidget::Score score)
{
    switch (score) {
    case ScoreWidget::Alerting:
        return InfoLabelType::Warning;
    case ScoreWidget::Poor:
        return InfoLabelType::Error;
    case ScoreWidget::Excellent:
    case ScoreWidget::Good:
        break;
    }
    return InfoLabelType::Ok;
}

void ScoreWidget::setScore(ScoreWidget::Score score)
{
    switch (score) {
    case Excellent:
        m_text = Tr::tr("Excellent");
        break;
    case Good:
        m_text = Tr::tr("Good");
        break;
    case Alerting:
        m_text = Tr::tr("Alerting");
        break;
    case Poor:
        m_text = Tr::tr("Poor");
        break;
    }
    const InfoLabelType type = infoType(score);
    m_color = infoTypeForegroundColor(type);
    m_bgColor = infoTypeBackgroundColor(type);
    m_icon = infoTypeIconLarge(type).icon();
    updateGeometry();
    update();
}

QSize ScoreWidget::minimumSizeHint() const
{
    const QFontMetrics fm(scoreWidgetTf.font());
    const int width =
        SpacingTokens::PaddingHS
        + m_iconSize
        + SpacingTokens::GapHXs
        + fm.boundingRect(m_text).width()
        + SpacingTokens::PaddingHS;
    const int height =
        SpacingTokens::PaddingVXs
        + qMax(m_iconSize, scoreWidgetTf.lineHeight())
        + SpacingTokens::PaddingVXs;
    return {width, height};
}

class Category : public QWidget
{
public:
    Category(const QString &title, const QString &description, Size size = Small,
             QWidget *parent = nullptr);

    void setText(const QString &title, const QString &text);
    void setScore(ScoreWidget::Score score, int points, int pointsRange = 100);

private:
    QLabel *m_title;
    ScoreWidget *m_scoreWidget;
    QLabel *m_score;
    QLabel *m_description;
};

Category::Category(const QString &title, const QString &description, Size size, QWidget *parent)
    : QWidget(parent)
{
    const bool small = size == Small;

    m_title = new QLabel(title);
    applyTf(m_title, small ? titleSmallTf : titleTf, false);

    m_scoreWidget = new ScoreWidget;

    m_score = new QLabel;
    applyTf(m_score, small ? titleSmallTf : titleTf, false);

    m_description = new QLabel(description);
    applyTf(m_description, small ? textSmallTf : textTf, false);
    m_description->setWordWrap(true);

    using namespace Layouting;
    Column {
        customMargins(0, 0, 0, 0),
        spacing(small ? SpacingTokens::GapVM : SpacingTokens::GapVL),
        m_title,
        Row { st, m_scoreWidget, st },
        m_score,
        m_description,
        st,
    }.attachTo(this);
}

void Category::setScore(ScoreWidget::Score score, int points, int pointsRange)
{
    m_scoreWidget->setScore(score);
    m_score->setText(QString::fromLatin1("%1/%2").arg(points).arg(pointsRange));
}

class Gauge : public QWidget
{
public:
    Gauge(QWidget *parent = nullptr);

    void setMinValue(int value);
    void setMaxValue(int value);
    void setValue(int value);
    void setUnit(const QString &unit);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QConicalGradient gradient() const;

    const int m_arcSpanDegrees = 270;
    int m_minValue = 0;
    int m_maxValue = 100;
    int m_value = 50;
    QString m_unit;
};

Gauge::Gauge(QWidget *parent)
    : QWidget(parent)
{
}

void Gauge::setMinValue(int value)
{
    m_minValue = value;
    update();
}

void Gauge::setMaxValue(int value)
{
    m_maxValue = value;
    update();
}

void Gauge::setValue(int value)
{
    m_value = value;
    update();
}

void Gauge::setUnit(const QString &unit)
{
    m_unit = unit;
    update();
}

static qreal inbetweenRatio(qreal min, qreal max, qreal ratio)
{
    return min + ratio * (max - min);
}

QConicalGradient Gauge::gradient() const
{
    const qreal coneAngleSafetyTweak = 1.0;
    const qreal coneStartAngle = 90 - m_arcSpanDegrees / 2.0 - coneAngleSafetyTweak;
    const qreal coneEndAngle = m_arcSpanDegrees + 2 * coneAngleSafetyTweak;
    QConicalGradient gradient(rect().center(), coneStartAngle);
    const qreal gradientStart = 0.0;
    const qreal gradientEnd = 1.0 / 360.0 * coneEndAngle;
    const QColor success = creatorColor(Theme::Token_Notification_Success_Muted);
    const QColor alert = creatorColor(Theme::Token_Notification_Alert_Muted);
    const QColor danger = creatorColor(Theme::Token_Notification_Danger_Muted);
    gradient.setColorAt(gradientStart, success);
    gradient.setColorAt(inbetweenRatio(gradientStart, gradientEnd, 0.10), success);
    gradient.setColorAt(inbetweenRatio(gradientStart, gradientEnd, 0.30), alert);
    gradient.setColorAt(inbetweenRatio(gradientStart, gradientEnd, 0.50), alert);
    gradient.setColorAt(inbetweenRatio(gradientStart, gradientEnd, 0.70), danger);
    gradient.setColorAt(gradientEnd, danger);
    return gradient;
}

void Gauge::paintEvent(QPaintEvent *event)
{
    const int side = qMin(width(), height());
    const QRect gaugeRect((width() - side) / 2, (height() - side) / 2, side, side);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int degreeUnit = 16; // QPainter arc angles are in 1/16th of a degree
    const int arcWidth = qMax(4, side / 11); // Ca 10px with 110px widget size
    const int arcMargin = ceil(arcWidth / 2.0);
    const QRect arcRect = gaugeRect.adjusted(arcMargin, arcMargin, -arcMargin, -arcMargin);
    const int startAngle = (m_arcSpanDegrees - (360 + m_arcSpanDegrees) / 2 - 90);

    QConicalGradient gradient = this->gradient();
    gradient.setCenter(gaugeRect.center());
    const QPen bgPen(gradient, arcWidth, Qt::SolidLine, Qt::FlatCap);
    painter.setPen(bgPen);
    painter.setOpacity(0.25);
    painter.drawArc(arcRect, startAngle * degreeUnit, - m_arcSpanDegrees * degreeUnit);
    painter.setOpacity(1.0);
    if (m_maxValue > m_minValue) {
        const double ratio = qBound(0.0, double(m_value - m_minValue)
                                    / double(m_maxValue - m_minValue), 1.0);
        const int valueSpan = qRound(-m_arcSpanDegrees * ratio);
        painter.drawArc(arcRect, startAngle * degreeUnit, valueSpan * degreeUnit);
    }

    const QString text = QString::number(m_value) + m_unit;
    constexpr TextFormat textTf {
        .themeColor = Theme::Token_Text_Default,
        .uiElement = UiElementH2,
        .drawTextFlags = Qt::AlignCenter,
    };
    painter.setFont(textTf.font());
    painter.setPen(textTf.color());
    painter.drawText(rect(), text, QTextOption(Qt::Alignment(textTf.drawTextFlags)));

    QWidget::paintEvent(event);
}

constexpr int findingIconSize = 24;

static InfoLabelType infoType(Finding::Severity severity)
{
    switch (severity) {
    case Finding::Critical:
        return InfoLabelType::Error;
    case Finding::Warning:
        return InfoLabelType::Warning;
    case Finding::Info:
        break;
    }
    return InfoLabelType::Information;
}

// Cost and occurrences, as far as the finding provides them.
static QString findingMetrics(const QModelIndex &index)
{
    QStringList metrics;
    const QString cost =
        index.siblingAtColumn(QmlProfilerFindingsModel::ColumnCost).data().toString();
    if (!cost.isEmpty())
        metrics.append(cost);
    const int occurrences =
        index.siblingAtColumn(QmlProfilerFindingsModel::ColumnOccurrences).data().toInt();
    if (occurrences > 0)
        metrics.append(Tr::tr("%n occurrence(s)", nullptr, occurrences));
    return metrics.join(", ");
}

class FindingHeader : public QWidget
{
    Q_OBJECT

public:
    FindingHeader(QWidget *parent = nullptr);

signals:
    void clicked();

protected:
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
};

FindingHeader::FindingHeader(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_Hover);
    setCursor(Qt::PointingHandCursor);
}

void FindingHeader::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint()))
        emit clicked();
    QWidget::mouseReleaseEvent(event);
}

void FindingHeader::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    update();
}

void FindingHeader::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    update();
}

void FindingHeader::paintEvent([[maybe_unused]] QPaintEvent *event)
{
    if (!underMouse())
        return;
    QPainter painter(this);
    drawCardBg(&painter, rect(), creatorColor(Theme::Token_Foreground_Subtle));
}

class FindingItemWidget : public QWidget
{
    Q_OBJECT

public:
    FindingItemWidget(QWidget *parent = nullptr);

    void setFinding(const QModelIndex &index);

signals:
    void activated(const QModelIndex &index);

private:
    QPersistentModelIndex m_index;
    QLabel *m_icon = nullptr;
    QLabel *m_finding = nullptr;
    QLabel *m_details = nullptr;
};

FindingItemWidget::FindingItemWidget(QWidget *parent)
    : QWidget(parent)
{
    m_icon = new QLabel;
    m_icon->setFixedSize(findingIconSize, findingIconSize);

    m_finding = new QLabel;
    m_finding->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    m_finding->setTextInteractionFlags(Qt::NoTextInteraction);
    m_finding->setWordWrap(true);

    m_details = new QLabel;
    m_details->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_details->setWordWrap(true);

    auto header = new FindingHeader;
    connect(header, &FindingHeader::clicked, this, [this] {
        if (m_index.isValid())
            emit activated(m_index);
    });

    using namespace Layouting;
    Row {
        customMargins(0, SpacingTokens::PaddingVS, 0, SpacingTokens::PaddingVS),
        m_finding,
    }.attachTo(header);

    Row {
        customMargins(0, 0,
                      SpacingTokens::PaddingHS, SpacingTokens::PaddingVS),
        spacing(0),
        Column {
            customMargins(SpacingTokens::PaddingHS, SpacingTokens::PaddingVS,
                          SpacingTokens::PaddingHS, SpacingTokens::PaddingVS),
            m_icon,
            st,
        },
        Column {
            header,
            m_details,
        },
    }.attachTo(this);

    QSizePolicy policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    policy.setHeightForWidth(true); // The details wraps.
    setSizePolicy(policy);
}

void FindingItemWidget::setFinding(const QModelIndex &index)
{
    m_index = index;

    const auto severity = Finding::Severity(
        index.siblingAtColumn(QmlProfilerFindingsModel::ColumnSeverity)
            .data(QmlProfilerFindingsModel::SortRole).toInt());
    const QIcon icon = Utils::infoTypeIconLarge(infoType(severity)).icon();
    m_icon->setPixmap(icon.pixmap(QSize(findingIconSize, findingIconSize), devicePixelRatioF()));

    const QString captionFontCss = fontToCssProperties(uiFont(UiElementCaption));
    const QString captionStrongFontCss = fontToCssProperties(uiFont(UiElementCaptionStrong));

    auto styledSpanHtml = [](const QString &style, const QString &text) {
        return QString("<span style=\"%1\">%2</span>").arg(style, text.toHtmlEscaped());
    };

    auto titleAndDescriptionHtml = [&](const QString &title, const QString &description) {
        return styledSpanHtml(captionStrongFontCss, title) + " "
               + styledSpanHtml(captionFontCss, description);
    };

    const QString location = index.siblingAtColumn(QmlProfilerFindingsModel::ColumnLocation)
                                 .data().toString();
    const QString finding = index.siblingAtColumn(QmlProfilerFindingsModel::ColumnFinding)
                                .data().toString();
    const QString findingHtml = QString("<table width=\"100%\">"
                                        "<tr><td>%1</td><td align=\"right\">%2</td></tr>"
                                        "</table>")
                                    .arg(titleAndDescriptionHtml(finding, location),
                                         styledSpanHtml(captionFontCss, findingMetrics(index)));;
    m_finding->setText(findingHtml);

    const QString why = index.data(QmlProfilerFindingsModel::WhyRole).toString();
    const QString suggestion = index.data(QmlProfilerFindingsModel::SuggestionRole).toString();
    QStringList detailsHtml;
    if (!why.isEmpty())
        detailsHtml.append(titleAndDescriptionHtml(Tr::tr("Why:"), why));
    if (!suggestion.isEmpty())
        detailsHtml.append(titleAndDescriptionHtml(Tr::tr("Suggestion:"), suggestion));
    const bool hasDetails = !detailsHtml.isEmpty();
    if (hasDetails)
        m_details->setText("<table width=\"100%\"><tr><td>"
                           + detailsHtml.join("<br/>")
                           + "</td></tr></table>");
    m_details->setVisible(hasDetails);
}

class FindingsView : public QtcSeparatedItemsWidget
{
    Q_OBJECT

public:
    FindingsView(QmlProfilerFindingsModel *model, QWidget *parent = nullptr);

signals:
    void activated(const QModelIndex &index);

private:
    void updateFindings();

    const int m_maxVisibleFindings = 5;
    QmlProfilerFindingsModel *m_model = nullptr;
    QList<FindingItemWidget *> m_findingsWidgets;
};

FindingsView::FindingsView(QmlProfilerFindingsModel *model, QWidget *parent)
    : QtcSeparatedItemsWidget(parent)
    , m_model(model)
{
    setSeparatorInset(0);

    using namespace Layouting;
    Column column {
        noMargin,
        spacing(QtcSeparatedItemsWidget::separatorLineWidth()),
    };
    for (int i = 0; i < m_maxVisibleFindings; ++i) {
        auto findingsWidget = new FindingItemWidget;
        connect(findingsWidget, &FindingItemWidget::activated, this, &FindingsView::activated);
        m_findingsWidgets.append(findingsWidget);
        column.addItem(findingsWidget);
    }
    column.attachTo(this);

    QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    setSizePolicy(policy);

    connect(m_model, &QAbstractItemModel::modelReset, this, &FindingsView::updateFindings);
    updateFindings();
}

void FindingsView::updateFindings()
{
    const int findings = qMin(m_model->rowCount(), m_maxVisibleFindings);
    for (int i = 0; i < m_findingsWidgets.count(); ++i) {
        FindingItemWidget *findingsWidget = m_findingsWidgets.at(i);
        const bool hasFinding = i < findings;
        if (hasFinding)
            findingsWidget->setFinding(m_model->index(i, QmlProfilerFindingsModel::ColumnFinding));
        findingsWidget->setVisible(hasFinding);
    }
}

class QmlProfilerDashboardViewPrivate : public QObject
{
public:
    QmlProfilerDashboardViewPrivate(QObject *parent = nullptr);

    bool hasData() const;
    void updateVisibility();

    bool isFirstFinalize = true;

    QmlProfilerDashboardStats *stats = nullptr;

    Category *overallRating = nullptr;

    QLabel *gaugeTitle = nullptr;
    Gauge *gauge = nullptr;
    QLabel *gaugeText = nullptr;

    QLabel *framesTitle = nullptr;
    QtcBadge *framesOnTargetBadge = nullptr;
    QLabel *framesOnTargetLabel = nullptr;
    QtcBadge *framesNearTargetBadge = nullptr;
    QLabel *framesNearTargetLabel = nullptr;
    QtcBadge *framesFailedBadge = nullptr;
    QLabel *framesFailedLabel = nullptr;
    QLabel *framesText = nullptr;

    QLabel *categoriesTitle = nullptr;
    Category *uiResponsiveness = nullptr;
    Category *frameConsistency = nullptr;
    Category *stutterPrevention = nullptr;
    Category *p99Quality = nullptr;
    Category *startupSpeed = nullptr;

    QLabel *findingsTitle = nullptr;
    QmlProfilerFindingsModel *findingsModel = nullptr;
    FindingsView *findingsView = nullptr;
    QWidget *statsSection = nullptr;
    QtcRectangleWidget *findingsSection = nullptr;
    QWidget *noDataAvailableSection = nullptr;
};

QmlProfilerDashboardViewPrivate::QmlProfilerDashboardViewPrivate(QObject *parent)
    : QObject(parent)
{
}

bool QmlProfilerDashboardViewPrivate::hasData() const
{
    return stats->hasData() || findingsModel->rowCount() > 0;
}

void QmlProfilerDashboardViewPrivate::updateVisibility()
{
    statsSection->setVisible(stats->hasData());
    findingsSection->setVisible(findingsModel->rowCount() > 0);
    noDataAvailableSection->setVisible(!hasData());
}

QmlProfilerDashboardView::QmlProfilerDashboardView(QmlProfilerModelManager *manager,
                                                   QWidget *parent)
    : QWidget(parent)
    , d(new QmlProfilerDashboardViewPrivate(this))
{
    d->stats = new QmlProfilerDashboardStats(manager, d);
    connect(d->stats, &QmlProfilerDashboardStats::changed, this, [this] {
        d->updateVisibility();
        updateValues();
    });

    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);
    setObjectName("QmlProfiler.Dashboard.Dock");
    setWindowTitle(Tr::tr("Dashboard"));


    // Performance Rating
    d->overallRating = new Category(
        Tr::tr("Performance Rating"),
        Tr::tr("Overall technical rating of the application performance"),
        Large);


    // No data
    QLabel *noDataText = new QLabel(
        Tr::tr("There is nothing to show.\n"
               "Either no trace has been taken, or it contains no animation events or findings."));
    applyTf(noDataText, textTf, false);
    noDataText->setWordWrap(true);


    // Frame Rate
    d->gaugeTitle = new QLabel(Tr::tr("Frame Rate"));
    applyTf(d->gaugeTitle, titleTf);
    d->gauge = new Gauge;
    d->gauge->setFixedSize(110, 110);
    d->gauge->setUnit("%");
    d->gaugeText = new QLabel(
        Tr::tr("%1+ FPS (Steady State)").arg(qRound(kDisplayRefreshRate)));
    applyTf(d->gaugeText, textTf);


    // Frame Time Analysis
    d->framesTitle = new QLabel(Tr::tr("Frame Time Analysis"));
    applyTf(d->framesTitle, titleTf);

    d->framesOnTargetBadge = new QtcBadge;
    d->framesOnTargetBadge->setInfoType(Utils::InfoLabelType::Ok);
    d->framesOnTargetLabel = new QLabel(
        Tr::tr("On Target (%1+ FPS, <= %2 ms)")
            .arg(qRound(kDisplayRefreshRate))
            .arg(kOnTargetFrameTimeMs, 0, 'f', 1));
    applyTf(d->framesOnTargetLabel, textFrameAnalysisTf);

    d->framesNearTargetBadge = new QtcBadge;
    d->framesNearTargetBadge->setInfoType(Utils::InfoLabelType::Warning);
    d->framesNearTargetLabel = new QLabel(
        Tr::tr("Near Target (%1-%2 ms)")
            .arg(kOnTargetFrameTimeMs, 0, 'f', 1)
            .arg(kNearTargetFrameTimeMs, 0, 'f', 1));
    applyTf(d->framesNearTargetLabel, textFrameAnalysisTf);

    d->framesFailedBadge = new QtcBadge;
    d->framesFailedBadge->setInfoType(Utils::InfoLabelType::NotOk);
    d->framesFailedLabel = new QLabel(
        Tr::tr("Failed (< %1 FPS, > %2 ms)")
            .arg(qRound(1000.0 / kNearTargetFrameTimeMs))
            .arg(kNearTargetFrameTimeMs, 0, 'f', 1));
    applyTf(d->framesFailedLabel, textFrameAnalysisTf);

    d->framesText = new QLabel(
        Tr::tr("%1+ FPS (Steady State)").arg(qRound(kDisplayRefreshRate)));
    applyTf(d->framesText, textTf);


    // Multi-factor Performance Analysis
    d->categoriesTitle = new QLabel(Tr::tr("Multifactor Performance Analysis"));
    applyTf(d->categoriesTitle, titleTf);

    d->uiResponsiveness = new Category(
        Tr::tr("UI Responsiveness"),
        Tr::tr("Percentage of frames within the P95 threshold for smooth UI interactions"));
    d->frameConsistency = new Category(
        Tr::tr("Frame Consistency"),
        Tr::tr("Frame time variation (lower is better for smooth animations)"));
    d->stutterPrevention = new Category(
        Tr::tr("Stutter Prevention"),
        Tr::tr("Frames slower than %1 FPS (%2 ms) cause noticeable UI freezes")
            .arg(qRound(kStutterFps))
            .arg(kStutterFrameTimeMs, 0, 'f', 1));
    d->p99Quality = new Category(
        Tr::tr("P99 Quality"),
        Tr::tr("99th percentile frame time ensures a consistent experience"));
    d->startupSpeed = new Category(
        Tr::tr("Startup Speed"),
        Tr::tr("Frames until reaching steady-state performance"));


    // Findings
    d->findingsTitle = new QLabel(Tr::tr("Findings"));
    applyTf(d->findingsTitle, titleTf);

    d->findingsModel = new QmlProfilerFindingsModel(manager);
    d->findingsModel->setParent(d);
    d->findingsView = new FindingsView(d->findingsModel);

    // An empty findings card would read as a broken one, so the section only exists once
    // the trace has produced findings.
    connect(d->findingsModel, &QAbstractItemModel::modelReset, this, [this] {
        d->updateVisibility();
        d->findingsView->updateGeometry();
    });

    // Registered after the stats and the findings, so that both have settled.
    manager->registerFeatures(0, {}, [this] {
        if (!d->isFirstFinalize)
            return;
        d->isFirstFinalize = false;
        if (!d->hasData())
            emit noDataShown();
    }, {});
    // Not in the clearer: restrictByFilter() runs it, too, without starting a new trace.
    connect(manager, &QmlProfilerModelManager::eventsCleared, this, [this] {
        d->isFirstFinalize = true;
    });

    connect(d->findingsView, &FindingsView::activated,
            this, [this](const QModelIndex &index) {
        if (findingIsInSource(index)) {
            emit gotoSourceLocation(index.data(QmlProfilerFindingsModel::FilenameRole).toString(),
                                    index.data(QmlProfilerFindingsModel::LineRole).toInt(),
                                    index.data(QmlProfilerFindingsModel::ColumnRole).toInt());
        }

        const int typeIndex = index.data(QmlProfilerFindingsModel::TypeIdRole).toInt();
        if (typeIndex != -1)
            emit typeSelected(typeIndex);
    });


    const QBrush rectFillBrush = creatorColor(Theme::Token_Background_Muted);
    const QPen rectStrokePen = creatorColor(Theme::Token_Stroke_Subtle);
    using namespace Layouting;

    auto framesGrid = Grid {
        noMargin,
        spacing(SpacingTokens::GapVL),
        d->framesOnTargetBadge, d->framesOnTargetLabel, br,
        d->framesNearTargetBadge, d->framesNearTargetLabel, br,
        d->framesFailedBadge, d->framesFailedLabel, br,
    }.emerge();
    framesGrid->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    Column {
        noMargin,
        ScrollArea {
            fixSizeHintBug(true),
            frameShape(QFrame::NoFrame),
            Column {
                st,
                customMargins(SpacingTokens::GapVXxl, SpacingTokens::GapVXxl,
                              SpacingTokens::GapVXxl, SpacingTokens::GapVXxl),
                spacing(SpacingTokens::GapVXxl),
                Widget {
                    bindTo(&d->noDataAvailableSection),
                    Column {
                        noMargin,
                        noDataText,
                    },
                },
                Widget {
                    bindTo(&d->statsSection),
                    Column {
                        noMargin,
                        spacing(SpacingTokens::GapVXxl),
                        Row {
                            QtDesignWidgets::Rectangle {
                                fillBrush(rectFillBrush),
                                strokePen(rectStrokePen),
                                Column {
                                    d->overallRating,
                                    st,
                                },
                            },
                            QtDesignWidgets::Rectangle {
                                fillBrush(rectFillBrush),
                                strokePen(rectStrokePen),
                                Column {
                                    spacing(0),
                                    d->gaugeTitle,
                                    Space(SpacingTokens::GapVL),
                                    Row { st, d->gauge, st },
                                    d->gaugeText,
                                },
                            },
                            QtDesignWidgets::Rectangle {
                                fillBrush(rectFillBrush),
                                strokePen(rectStrokePen),
                                Column {
                                    spacing(SpacingTokens::GapVL),
                                    d->framesTitle,
                                    Row { st, framesGrid, st },
                                    d->framesText,
                                    st,
                                },
                            },
                        },
                        QtDesignWidgets::Rectangle {
                            fillBrush(rectFillBrush),
                            strokePen(rectStrokePen),
                            Column {
                                spacing(SpacingTokens::GapVXl),
                                d->categoriesTitle,
                                QtDesignWidgets::SeparatedItems {
                                    separatorInset(0),
                                    Row {
                                        noMargin,
                                        spacing(2 * SpacingTokens::GapHXl
                                                + QtcSeparatedItemsWidget::separatorLineWidth()),
                                        d->uiResponsiveness,
                                        d->frameConsistency,
                                        d->stutterPrevention,
                                        d->p99Quality,
                                        d->startupSpeed,
                                    },
                                },
                            }
                        },
                    },
                },
                st,
                Row {
                    QtDesignWidgets::Rectangle {
                        bindTo(&d->findingsSection),
                        fillBrush(rectFillBrush),
                        strokePen(rectStrokePen),
                        Column {
                            spacing(0),
                            d->findingsTitle,
                            d->findingsView,
                        },
                    },
                },
                st,
            },
        },
    }.attachTo(this);

    d->updateVisibility();
    updateValues();
}

void QmlProfilerDashboardView::updateValues()
{
    d->gauge->setValue(d->stats->onTargetPercent());

    d->framesOnTargetBadge->setText(QString::number(d->stats->framesOnTarget()));
    d->framesNearTargetBadge->setText(QString::number(d->stats->framesNearTarget()));
    d->framesFailedBadge->setText(QString::number(d->stats->framesFailed()));

    const auto scoreFor = [](int percent) {
        if (percent >= 95)
            return ScoreWidget::Excellent;
        if (percent >= 80)
            return ScoreWidget::Good;
        if (percent >= 50)
            return ScoreWidget::Alerting;
        return ScoreWidget::Poor;
    };

    const int onTargetPercent = d->stats->onTargetPercent();
    const int stutterFreePercent = d->stats->stutterFreePercent();
    const int uiResponsivenessPercent = d->stats->uiResponsivenessPercent();
    const int p99Percent = d->stats->p99Percent();
    const int startupSpeedPercent = d->stats->startupSpeedPercent();
    const int overallPercent = d->stats->overallPercent();

    d->frameConsistency->setScore(scoreFor(onTargetPercent), onTargetPercent);
    d->stutterPrevention->setScore(scoreFor(stutterFreePercent), stutterFreePercent);
    d->uiResponsiveness->setScore(scoreFor(uiResponsivenessPercent), uiResponsivenessPercent);
    d->p99Quality->setScore(scoreFor(p99Percent), p99Percent);
    d->startupSpeed->setScore(scoreFor(startupSpeedPercent), startupSpeedPercent);
    d->overallRating->setScore(scoreFor(overallPercent), overallPercent);
}

} // namespace Profiler::Internal

#include "qmlprofilerdashboardview.moc"
