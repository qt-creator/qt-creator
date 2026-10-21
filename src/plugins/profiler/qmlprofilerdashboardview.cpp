// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerdashboardview.h"

#include "profilertr.h"
#include "qmlprofilerdashboardstats.h"
#include "qmlprofilerfindingsmodel.h"
#include "qmlprofilermodelmanager.h"
#include "qmlprofilersettings.h"

#include <tracing/timelineformatdata.h>

#include <utils/icon.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/qtdesignwidgets.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QComboBox>
#include <QEnterEvent>
#include <QFrame>
#include <QGridLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QVBoxLayout>

#include <optional>

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
    ScoreWidget(QWidget *parent = nullptr);

    void setRating(ScoreRating rating);

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
    setRating(ScoreRating::Excellent);
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

static InfoLabelType infoType(ScoreRating rating)
{
    switch (rating) {
    case ScoreRating::Alerting:
        return InfoLabelType::Warning;
    case ScoreRating::Poor:
        return InfoLabelType::Error;
    case ScoreRating::Excellent:
    case ScoreRating::Good:
        break;
    }
    return InfoLabelType::Ok;
}

static QString ratingName(ScoreRating rating)
{
    switch (rating) {
    case ScoreRating::Excellent:
        return Tr::tr("Excellent");
    case ScoreRating::Good:
        return Tr::tr("Good");
    case ScoreRating::Alerting:
        return Tr::tr("Alerting");
    case ScoreRating::Poor:
        return Tr::tr("Poor");
    }
    return {};
}

void ScoreWidget::setRating(ScoreRating rating)
{
    m_text = ratingName(rating);
    const InfoLabelType type = infoType(rating);
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

    void setScore(int score);
    void setDescription(const QString &description);

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
    m_score->setObjectName("score");
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

void Category::setDescription(const QString &description)
{
    m_description->setText(description);
}

void Category::setScore(int score)
{
    m_scoreWidget->setRating(scoreRating(score));
    m_score->setText(QString::number(score));
}

static QString factorTitle(ScoreFactor::Id id)
{
    switch (id) {
    case ScoreFactor::UiResponsiveness:
        return Tr::tr("UI Responsiveness");
    case ScoreFactor::FrameConsistency:
        return Tr::tr("Frame Consistency");
    case ScoreFactor::StutterPrevention:
        return Tr::tr("Stutter Prevention");
    case ScoreFactor::P99Quality:
        return Tr::tr("P99 Quality");
    case ScoreFactor::StartupSpeed:
        return Tr::tr("Startup Speed");
    }
    return {};
}

// Rated against the target frame rate, so each description names the budget it applies.
static QString factorDescription(ScoreFactor::Id id, const QmlProfilerDashboardStats &stats)
{
    switch (id) {
    case ScoreFactor::UiResponsiveness:
        return Tr::tr("Share of frames on the %1 FPS target, with frames near it counting half")
            .arg(stats.targetFrameRate());
    case ScoreFactor::FrameConsistency:
        return Tr::tr("Share of frames on the %1 FPS target (<= %2 ms)")
            .arg(stats.targetFrameRate())
            .arg(stats.onTargetFrameTimeMs(), 0, 'f', 1);
    case ScoreFactor::StutterPrevention:
        return Tr::tr("Share of frames faster than half the target, %1 FPS (%2 ms). Slower "
                      "frames cause noticeable UI freezes")
            .arg(qRound(1000.0 / stats.stutterFrameTimeMs()))
            .arg(stats.stutterFrameTimeMs(), 0, 'f', 1);
    case ScoreFactor::P99Quality:
        return Tr::tr("Stands in for the 99th percentile frame time: failed frames "
                      "(> %1 ms) count %2 times")
            .arg(stats.nearTargetFrameTimeMs(), 0, 'f', 1)
            .arg(kP99FailedFrameWeight);
    case ScoreFactor::StartupSpeed:
        return Tr::tr("Stands in for the time to steady state: share of frames on target "
                      "minus share of stuttering frames");
    }
    return {};
}

static QString factorMeasurement(ScoreFactor::Id id, const QmlProfilerDashboardStats &stats)
{
    const int total = stats.framesTotal();
    switch (id) {
    case ScoreFactor::UiResponsiveness:
        return Tr::tr("%1 on target, %2 near target of %n frame(s)", nullptr, total)
            .arg(stats.framesOnTarget())
            .arg(stats.framesNearTarget());
    case ScoreFactor::FrameConsistency:
        return Tr::tr("%1 of %n frame(s) on target", nullptr, total).arg(stats.framesOnTarget());
    case ScoreFactor::StutterPrevention:
        return Tr::tr("%1 of %n frame(s) stuttering", nullptr, total)
            .arg(stats.framesStuttering());
    case ScoreFactor::P99Quality:
        return Tr::tr("%1 of %n frame(s) failed", nullptr, total).arg(stats.framesFailed());
    case ScoreFactor::StartupSpeed:
        return Tr::tr("%1% on target, %2% stuttering")
            .arg(stats.onTargetPercent())
            .arg(kMaxScore - stats.stutterFreePercent());
    }
    return {};
}

static QString scoreHelpText()
{
    return Tr::tr("The performance score is the sum of the points of five factors. Each factor "
                  "rates the frame times of the GUI thread from 0 (worst) to 100 (best) and "
                  "contributes up to %1 points, its score divided by %2. Findings do not "
                  "affect the score.")
        .arg(ScoreFactor::MaxPoints)
        .arg(kMaxScore / ScoreFactor::MaxPoints);
}

static QString signedPoints(int points)
{
    return points > 0 ? QChar(0x2212) + QString::number(points) : QString::number(points);
}

constexpr TextFormat breakdownHeaderTf {
    .themeColor = Theme::Token_Text_Muted,
    .uiElement = UiElementCaptionStrong,
    .drawTextFlags = Qt::AlignLeft | Qt::AlignVCenter,
};

constexpr TextFormat breakdownTf {
    .themeColor = Theme::Token_Text_Default,
    .uiElement = UiElementBody2,
    .drawTextFlags = Qt::AlignLeft | Qt::AlignVCenter,
};

constexpr TextFormat breakdownMutedTf {
    .themeColor = Theme::Token_Text_Muted,
    .uiElement = breakdownTf.uiElement,
    .drawTextFlags = breakdownTf.drawTextFlags,
};

constexpr TextFormat breakdownNumberTf {
    .themeColor = breakdownTf.themeColor,
    .uiElement = breakdownTf.uiElement,
    .drawTextFlags = Qt::AlignRight | Qt::AlignVCenter,
};

constexpr TextFormat breakdownTotalTf {
    .themeColor = Theme::Token_Text_Default,
    .uiElement = UiElementH6,
    .drawTextFlags = Qt::AlignRight | Qt::AlignVCenter,
};

constexpr int breakdownIconSize = 16;

// Lists the factors of the performance score with the points each one contributes, and
// highlights those rated below "Excellent".
class ScoreBreakdownView : public QWidget
{
public:
    ScoreBreakdownView(QWidget *parent = nullptr);

    void setStats(const QmlProfilerDashboardStats &stats);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    enum Column {
        IconColumn,
        TitleColumn,
        MeasurementColumn,
        ScoreColumn,
        PointsColumn,
        PointsLostColumn,
        ColumnCount,
    };

    class FactorRow
    {
    public:
        QLabel *icon = nullptr;
        QLabel *title = nullptr;
        QLabel *measurement = nullptr;
        QLabel *score = nullptr;
        QLabel *points = nullptr;
        QLabel *pointsLost = nullptr;
        std::optional<InfoLabelType> highlight;
    };

    QGridLayout *m_grid = nullptr;
    QList<FactorRow> m_rows;
    QLabel *m_totalPoints = nullptr;
    QLabel *m_totalPointsLost = nullptr;
};

ScoreBreakdownView::ScoreBreakdownView(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("QmlProfiler.Dashboard.ScoreBreakdown");

    m_grid = new QGridLayout(this);
    m_grid->setContentsMargins(SpacingTokens::PaddingHS, 0, SpacingTokens::PaddingHS, 0);
    m_grid->setHorizontalSpacing(SpacingTokens::GapHL);
    m_grid->setVerticalSpacing(SpacingTokens::GapVS);
    m_grid->setColumnStretch(MeasurementColumn, 1);

    const auto addLabel = [this](int row, int column, const TextFormat &tf,
                                 const QString &text = {}) {
        auto label = new QLabel(text);
        applyTf(label, tf);
        m_grid->addWidget(label, row, column);
        return label;
    };

    int row = 0;
    addLabel(row, TitleColumn, breakdownHeaderTf, Tr::tr("Factor"));
    addLabel(row, MeasurementColumn, breakdownHeaderTf, Tr::tr("Measured"));
    addLabel(row, ScoreColumn, breakdownHeaderTf, Tr::tr("Score"))
        ->setToolTip(Tr::tr("From 0 (worst) to 100 (best). Below %1, the factor is rated "
                            "lower than \"Excellent\" and highlighted.").arg(kExcellentScore));
    addLabel(row, PointsColumn, breakdownHeaderTf, Tr::tr("Points"))
        ->setToolTip(Tr::tr("The factor's score divided by %1, rounded.")
                         .arg(kMaxScore / ScoreFactor::MaxPoints));
    addLabel(row, PointsLostColumn, breakdownHeaderTf, Tr::tr("Lost"))
        ->setToolTip(Tr::tr("Points below the maximum of %1 per factor.")
                         .arg(ScoreFactor::MaxPoints));

    for (int id = 0; id < ScoreFactor::Count; ++id) {
        ++row;
        FactorRow factorRow;
        factorRow.icon = new QLabel;
        factorRow.icon->setObjectName(QString("status%1").arg(id));
        factorRow.icon->setFixedSize(breakdownIconSize, breakdownIconSize);
        m_grid->addWidget(factorRow.icon, row, IconColumn);
        factorRow.title = addLabel(row, TitleColumn, breakdownTf,
                                   factorTitle(ScoreFactor::Id(id)));
        factorRow.measurement = addLabel(row, MeasurementColumn, breakdownMutedTf);
        factorRow.score = addLabel(row, ScoreColumn, breakdownNumberTf);
        factorRow.points = addLabel(row, PointsColumn, breakdownNumberTf);
        factorRow.points->setObjectName(QString("points%1").arg(id));
        factorRow.pointsLost = addLabel(row, PointsLostColumn, breakdownNumberTf);
        m_rows.append(factorRow);
    }

    ++row;
    addLabel(row, TitleColumn, breakdownTotalTf, Tr::tr("Performance score"))
        ->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_totalPoints = addLabel(row, PointsColumn, breakdownTotalTf);
    m_totalPoints->setObjectName("totalPoints");
    m_totalPointsLost = addLabel(row, PointsLostColumn, breakdownTotalTf);
}

void ScoreBreakdownView::setStats(const QmlProfilerDashboardStats &stats)
{
    const QList<ScoreFactor> factors = stats.scoreBreakdown();
    QTC_ASSERT(factors.size() == m_rows.size(), return);

    int totalPointsLost = 0;
    for (int i = 0; i < factors.size(); ++i) {
        const ScoreFactor &factor = factors.at(i);
        FactorRow &row = m_rows[i];
        QTC_CHECK(factor.id == i);

        if (factor.pullsScoreDown()) {
            const InfoLabelType type = infoType(scoreRating(factor.score));
            row.highlight = type == InfoLabelType::Ok ? InfoLabelType::Information : type;
        } else {
            row.highlight.reset();
        }
        const InfoLabelType iconType = row.highlight.value_or(InfoLabelType::Ok);
        row.icon->setPixmap(infoTypeIconLarge(iconType).icon().pixmap(
            QSize(breakdownIconSize, breakdownIconSize), devicePixelRatioF()));
        row.icon->setAccessibleName(row.highlight ? Tr::tr("Lowers the score")
                                                  : Tr::tr("Excellent"));

        row.title->setToolTip(factorDescription(factor.id, stats));
        row.measurement->setText(factorMeasurement(factor.id, stats));
        row.score->setText(QString::number(factor.score));
        row.points->setText(QString::number(factor.points));
        row.pointsLost->setText(signedPoints(factor.pointsLost()));
        totalPointsLost += factor.pointsLost();
    }
    m_totalPoints->setText(QString::number(stats.overallScore()));
    m_totalPointsLost->setText(signedPoints(totalPointsLost));
    update();
}

void ScoreBreakdownView::paintEvent([[maybe_unused]] QPaintEvent *event)
{
    QPainter painter(this);
    for (int i = 0; i < m_rows.size(); ++i) {
        const FactorRow &row = m_rows.at(i);
        if (!row.highlight)
            continue;
        const int gridRow = i + 1; // Below the header.
        const QRect rowRect = m_grid->cellRect(gridRow, IconColumn)
                                  .united(m_grid->cellRect(gridRow, ColumnCount - 1))
                                  .adjusted(-SpacingTokens::PaddingHS, -SpacingTokens::PaddingVXs,
                                            SpacingTokens::PaddingHS, SpacingTokens::PaddingVXs);
        drawCardBg(&painter, rowRect, creatorColor(infoTypeBackgroundColor(*row.highlight)));
    }
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
    void updateTargetFrameRate();

    bool isFirstFinalize = true;

    QmlProfilerModelManager *manager = nullptr;
    QmlProfilerDashboardStats *stats = nullptr;

    QComboBox *targetFrameRate = nullptr;

    Category *overallRating = nullptr;
    QtcButton *breakdownButton = nullptr;

    QWidget *breakdownSection = nullptr;
    QLabel *breakdownTitle = nullptr;
    ScoreBreakdownView *breakdown = nullptr;
    QLabel *slowestFrameLabel = nullptr;
    QtcButton *slowestFrameButton = nullptr;

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
    QList<Category *> factors; // By ScoreFactor::Id

    QLabel *findingsTitle = nullptr;
    QmlProfilerFindingsModel *findingsModel = nullptr;
    FindingsView *findingsView = nullptr;
    QWidget *targetFrameRateSection = nullptr;
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

// Everything rated against the target says which target that is, so that no number is read
// against a budget other than the one it was computed for.
void QmlProfilerDashboardViewPrivate::updateTargetFrameRate()
{
    const int target = globalSettings().targetFrameRate();
    stats->setTargetFrameRate(target);

    const int index = targetFrameRate->findData(target);
    if (index >= 0)
        targetFrameRate->setCurrentIndex(index);
    else
        targetFrameRate->setEditText(QString::number(target));

    gauge->setMaxValue(target);
    gaugeText->setText(Tr::tr("FPS measured (median)\nTarget: %1 FPS").arg(target));
    framesOnTargetLabel->setText(Tr::tr("On Target (>= %1 FPS, <= %2 ms)")
                                     .arg(target)
                                     .arg(stats->onTargetFrameTimeMs(), 0, 'f', 1));
    framesNearTargetLabel->setText(Tr::tr("Near Target (%1-%2 ms)")
                                       .arg(stats->onTargetFrameTimeMs(), 0, 'f', 1)
                                       .arg(stats->nearTargetFrameTimeMs(), 0, 'f', 1));
    framesFailedLabel->setText(Tr::tr("Failed (< %1 FPS, > %2 ms)")
                                   .arg(qRound(1000.0 / stats->nearTargetFrameTimeMs()))
                                   .arg(stats->nearTargetFrameTimeMs(), 0, 'f', 1));
    framesText->setText(Tr::tr("Measured frames, rated against the %1 FPS target").arg(target));
    breakdownTitle->setText(Tr::tr("Score Breakdown (Target: %1 FPS)").arg(target));
    for (int id = 0; id < factors.size(); ++id)
        factors.at(id)->setDescription(factorDescription(ScoreFactor::Id(id), *stats));
}

static void setTargetFrameRate(int framesPerSecond)
{
    QmlProfilerSettings &settings = globalSettings();
    // Through the volatile value and apply(), like the options page, so that everything
    // following the settings hears about it once.
    settings.targetFrameRate.setVolatileValue(framesPerSecond);
    settings.apply();
    settings.writeSettings();
}

void QmlProfilerDashboardViewPrivate::updateVisibility()
{
    targetFrameRateSection->setVisible(stats->hasData());
    statsSection->setVisible(stats->hasData());
    findingsSection->setVisible(findingsModel->rowCount() > 0);
    noDataAvailableSection->setVisible(!hasData());
}

QmlProfilerDashboardView::QmlProfilerDashboardView(QmlProfilerModelManager *manager,
                                                   QWidget *parent)
    : QWidget(parent)
    , d(new QmlProfilerDashboardViewPrivate(this))
{
    d->manager = manager;
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
        Tr::tr("Performance score from 0 to 100: the points of the five factors below, "
               "added up"),
        Large);
    d->overallRating->setObjectName("QmlProfiler.Dashboard.PerformanceRating");
    d->overallRating->setToolTip(scoreHelpText());

    d->breakdownButton = new QtcButton(Tr::tr("Show Breakdown"), QtcButton::SmallLink);
    d->breakdownButton->setCheckable(true);


    // Score Breakdown
    d->breakdownTitle = new QLabel(Tr::tr("Score Breakdown"));
    applyTf(d->breakdownTitle, titleTf);
    QLabel *breakdownHelp = new QLabel(scoreHelpText());
    applyTf(breakdownHelp, textFrameAnalysisTf, false);
    breakdownHelp->setWordWrap(true);
    d->breakdown = new ScoreBreakdownView;
    d->slowestFrameLabel = new QLabel;
    applyTf(d->slowestFrameLabel, textFrameAnalysisTf);
    d->slowestFrameButton = new QtcButton(Tr::tr("Show in Timeline"), QtcButton::SmallLink);
    d->slowestFrameButton->setObjectName("showSlowestFrame");
    connect(d->slowestFrameButton, &QAbstractButton::clicked, this, [this] {
        const qint64 end = d->stats->slowestFrameTimestamp();
        const qint64 duration = qRound64(d->stats->slowestFrameTimeMs() * 1e6);
        emit timelineRangeRequested(end - duration, end);
    });


    // Target frame rate
    const IntegerAspect &targetFrameRateAspect = globalSettings().targetFrameRate;
    auto targetFrameRateLabel = new QLabel(targetFrameRateAspect.labelText());
    applyTf(targetFrameRateLabel, textFrameAnalysisTf);
    d->targetFrameRate = new QComboBox;
    d->targetFrameRate->setObjectName("QmlProfiler.Dashboard.TargetFrameRate");
    d->targetFrameRate->setEditable(true);
    d->targetFrameRate->setInsertPolicy(QComboBox::NoInsert);
    d->targetFrameRate->setValidator(new QIntValidator(1, kMaxTargetFrameRate,
                                                      d->targetFrameRate));
    d->targetFrameRate->setToolTip(targetFrameRateAspect.toolTip());
    for (int preset : targetFrameRatePresets())
        d->targetFrameRate->addItem(QString::number(preset), preset);
    auto targetFrameRateUnit = new QLabel(Tr::tr("FPS"));
    applyTf(targetFrameRateUnit, textFrameAnalysisTf);

    // A preset is taken when it is picked; a typed value when it is confirmed.
    connect(d->targetFrameRate, &QComboBox::activated, this, [this](int index) {
        setTargetFrameRate(d->targetFrameRate->itemData(index).toInt());
    });
    connect(d->targetFrameRate->lineEdit(), &QLineEdit::editingFinished, this, [this] {
        bool ok = false;
        const int framesPerSecond = d->targetFrameRate->currentText().toInt(&ok);
        if (ok && framesPerSecond > 0)
            setTargetFrameRate(framesPerSecond);
        else
            d->updateTargetFrameRate();
    });


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
    d->gaugeText = new QLabel;
    d->gaugeText->setObjectName("gaugeText");
    applyTf(d->gaugeText, textTf, false);


    // Frame Time Analysis
    d->framesTitle = new QLabel(Tr::tr("Frame Time Analysis"));
    applyTf(d->framesTitle, titleTf);

    d->framesOnTargetBadge = new QtcBadge;
    d->framesOnTargetBadge->setInfoType(Utils::InfoLabelType::Ok);
    d->framesOnTargetLabel = new QLabel;
    applyTf(d->framesOnTargetLabel, textFrameAnalysisTf);

    d->framesNearTargetBadge = new QtcBadge;
    d->framesNearTargetBadge->setInfoType(Utils::InfoLabelType::Warning);
    d->framesNearTargetLabel = new QLabel;
    applyTf(d->framesNearTargetLabel, textFrameAnalysisTf);

    d->framesFailedBadge = new QtcBadge;
    d->framesFailedBadge->setInfoType(Utils::InfoLabelType::NotOk);
    d->framesFailedLabel = new QLabel;
    applyTf(d->framesFailedLabel, textFrameAnalysisTf);

    d->framesText = new QLabel;
    d->framesText->setObjectName("framesText");
    applyTf(d->framesText, textTf, false);
    d->framesText->setWordWrap(true);


    // Multi-factor Performance Analysis
    d->categoriesTitle = new QLabel(Tr::tr("Multifactor Performance Analysis"));
    applyTf(d->categoriesTitle, titleTf);

    for (int id = 0; id < ScoreFactor::Count; ++id) {
        // The description depends on the target: updateTargetFrameRate() sets it.
        auto factor = new Category(factorTitle(ScoreFactor::Id(id)), {});
        factor->setToolTip(Tr::tr("Score from 0 (worst) to 100 (best). Contributes up to "
                                  "%1 points to the performance score.")
                               .arg(ScoreFactor::MaxPoints));
        d->factors.append(factor);
    }


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

    Row factorsRow {
        noMargin,
        spacing(2 * SpacingTokens::GapHXl + QtcSeparatedItemsWidget::separatorLineWidth()),
    };
    for (Category *factor : std::as_const(d->factors))
        factorsRow.addItem(factor);

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
                    bindTo(&d->targetFrameRateSection),
                    Row {
                        noMargin,
                        spacing(SpacingTokens::GapHXs),
                        targetFrameRateLabel,
                        d->targetFrameRate,
                        targetFrameRateUnit,
                        st,
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
                                    Row { st, d->breakdownButton, st },
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
                            bindTo(&d->breakdownSection),
                            fillBrush(rectFillBrush),
                            strokePen(rectStrokePen),
                            Column {
                                spacing(SpacingTokens::GapVL),
                                d->breakdownTitle,
                                breakdownHelp,
                                d->breakdown,
                                Row {
                                    noMargin,
                                    spacing(SpacingTokens::GapHM),
                                    d->slowestFrameLabel,
                                    d->slowestFrameButton,
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
                                    factorsRow,
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

    d->breakdownSection->setVisible(false);
    connect(d->breakdownButton, &QAbstractButton::toggled, this, [this](bool checked) {
        d->breakdownSection->setVisible(checked);
        d->breakdownButton->setText(checked ? Tr::tr("Hide Breakdown")
                                            : Tr::tr("Show Breakdown"));
    });

    connect(&globalSettings().targetFrameRate, &BaseAspect::changed,
            d, &QmlProfilerDashboardViewPrivate::updateTargetFrameRate);
    d->updateTargetFrameRate();

    d->updateVisibility();
    updateValues();
}

void QmlProfilerDashboardView::updateValues()
{
    d->gauge->setValue(d->stats->medianFrameRate());

    d->framesOnTargetBadge->setText(QString::number(d->stats->framesOnTarget()));
    d->framesNearTargetBadge->setText(QString::number(d->stats->framesNearTarget()));
    d->framesFailedBadge->setText(QString::number(d->stats->framesFailed()));

    const QList<ScoreFactor> factors = d->stats->scoreBreakdown();
    QTC_ASSERT(factors.size() == d->factors.size(), return);
    for (const ScoreFactor &factor : factors)
        d->factors.at(factor.id)->setScore(factor.score);
    d->overallRating->setScore(d->stats->overallScore());
    d->breakdown->setStats(*d->stats);

    const bool hasSlowFrame = d->stats->slowestFrameTimestamp() >= 0
                              && d->stats->slowestFrameTimeMs() > d->stats->onTargetFrameTimeMs();
    if (hasSlowFrame) {
        d->slowestFrameLabel->setText(
            Tr::tr("Slowest frame: %1 ms at %2")
                .arg(d->stats->slowestFrameTimeMs(), 0, 'f', 1)
                .arg(Timeline::formatTime(d->stats->slowestFrameTimestamp()
                                          - d->manager->traceStart())));
    }
    d->slowestFrameLabel->setVisible(hasSlowFrame);
    d->slowestFrameButton->setVisible(hasSlowFrame);
}

} // namespace Profiler::Internal

#include "qmlprofilerdashboardview.moc"
