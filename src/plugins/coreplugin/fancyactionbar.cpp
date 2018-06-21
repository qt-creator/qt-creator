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

#include "fancyactionbar.h"

#include "coreconstants.h"

#include <utils/hostosinfo.h>
#include <utils/stringutils.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/tooltip/tooltip.h>

#include <QAction>
#include <QDebug>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmapCache>
#include <QPropertyAnimation>
#include <QStyle>
#include <QStyleOption>
#include <QVBoxLayout>

using namespace Utils;

namespace Core {
namespace Internal {

FancyToolButton::FancyToolButton(QAction *action, QWidget *parent)
    : QToolButton(parent)
{
    setDefaultAction(action);
    connect(action, &QAction::changed, this, &FancyToolButton::actionChanged);
    actionChanged();

    setAttribute(Qt::WA_Hover, true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

bool FancyToolButton::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::Enter: {
        auto animation = new QPropertyAnimation(this, "fader");
        animation->setDuration(125);
        animation->setEndValue(1.0);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    } break;
    case QEvent::Leave: {
        auto animation = new QPropertyAnimation(this, "fader");
        animation->setDuration(125);
        animation->setEndValue(0.0);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    } break;
    case QEvent::ToolTip: {
        auto he = static_cast<QHelpEvent *>(e);
        ToolTip::show(mapToGlobal(he->pos()), toolTip(), this);
        return true;
    }
    default:
        break;
    }
    return QToolButton::event(e);
}

static QVector<QString> splitInTwoLines(const QString &text,
                                        const QFontMetrics &fontMetrics,
                                        qreal availableWidth)
{
    // split in two lines.
    // this looks if full words can be split off at the end of the string,
    // to put them in the second line. First line is drawn with ellipsis,
    // second line gets ellipsis if it couldn't split off full words.
    QVector<QString> splitLines(2);
    const QRegExp rx(QLatin1String("\\s+"));
    int splitPos = -1;
    int nextSplitPos = text.length();
    do {
        nextSplitPos = rx.lastIndexIn(text, nextSplitPos - text.length() - 1);
        if (nextSplitPos != -1) {
            int splitCandidate = nextSplitPos + rx.matchedLength();
            if (fontMetrics.width(text.mid(splitCandidate)) <= availableWidth)
                splitPos = splitCandidate;
            else
                break;
        }
    } while (nextSplitPos > 0 && fontMetrics.width(text.left(nextSplitPos)) > availableWidth);
    // check if we could split at white space at all
    if (splitPos < 0) {
        splitLines[0] = fontMetrics.elidedText(text, Qt::ElideRight, int(availableWidth));
        QString common = Utils::commonPrefix(QStringList({splitLines[0], text}));
        splitLines[1] = text.mid(common.length());
        // elide the second line even if it fits, since it is cut off in mid-word
        while (fontMetrics.width(QChar(0x2026) /*'...'*/ + splitLines[1]) > availableWidth
               && splitLines[1].length() > 3
               /*keep at least three original characters (should not happen)*/) {
            splitLines[1].remove(0, 1);
        }
        splitLines[1] = QChar(0x2026) /*'...'*/ + splitLines[1];
    } else {
        splitLines[0] = fontMetrics.elidedText(text.left(splitPos).trimmed(),
                                               Qt::ElideRight,
                                               int(availableWidth));
        splitLines[1] = text.mid(splitPos);
    }
    return splitLines;
}

void FancyToolButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);

    // draw borders
    if (!HostOsInfo::isMacHost() // Mac UIs usually don't hover
        && m_fader > 0 && isEnabled() && !isDown() && !isChecked()) {
        painter.save();
        if (creatorTheme()->flag(Theme::FlatToolBars)) {
            const QColor hoverColor = creatorTheme()->color(Theme::FancyToolButtonHoverColor);
            QColor fadedHoverColor = hoverColor;
            fadedHoverColor.setAlpha(int(m_fader * hoverColor.alpha()));
            painter.fillRect(rect(), fadedHoverColor);
        } else {
            painter.setOpacity(m_fader);
            FancyToolButton::hoverOverlay(&painter, rect());
        }
        painter.restore();
    } else if (isDown() || isChecked()) {
        painter.save();
        const QColor selectedColor = creatorTheme()->color(Theme::FancyToolButtonSelectedColor);
        if (creatorTheme()->flag(Theme::FlatToolBars)) {
            painter.fillRect(rect(), selectedColor);
        } else {
            QLinearGradient grad(rect().topLeft(), rect().topRight());
            grad.setColorAt(0, Qt::transparent);
            grad.setColorAt(0.5, selectedColor);
            grad.setColorAt(1, Qt::transparent);
            painter.fillRect(rect(), grad);
            painter.setPen(QPen(grad, 1.0));
            const QRectF borderRectF(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5));
            painter.drawLine(borderRectF.topLeft(), borderRectF.topRight());
            painter.drawLine(borderRectF.topLeft(), borderRectF.topRight());
            painter.drawLine(borderRectF.topLeft() + QPointF(0, 1),
                             borderRectF.topRight() + QPointF(0, 1));
            painter.drawLine(borderRectF.bottomLeft(), borderRectF.bottomRight());
            painter.drawLine(borderRectF.bottomLeft(), borderRectF.bottomRight());
        }
        painter.restore();
    }

    const QIcon::Mode iconMode = isEnabled()
                                     ? ((isDown() || isChecked()) ? QIcon::Active : QIcon::Normal)
                                     : QIcon::Disabled;
    QRect iconRect(0, 0, Constants::MODEBAR_ICON_SIZE, Constants::MODEBAR_ICON_SIZE);

    const bool isTitledAction = defaultAction()->property("titledAction").toBool();
    // draw popup texts
    if (isTitledAction && !m_iconsOnly) {
        QFont normalFont(painter.font());
        QRect centerRect = rect();
        normalFont.setPointSizeF(StyleHelper::sidebarFontSize());
        QFont boldFont(normalFont);
        boldFont.setBold(true);
        const QFontMetrics fm(normalFont);
        const QFontMetrics boldFm(boldFont);
        const int lineHeight = boldFm.height();
        const int textFlags = Qt::AlignVCenter | Qt::AlignHCenter;

        const QString projectName = defaultAction()->property("heading").toString();
        if (!projectName.isNull())
            centerRect.adjust(0, lineHeight + 4, 0, 0);

        centerRect.adjust(0, 0, 0, -lineHeight * 2 - 4);

        iconRect.moveCenter(centerRect.center());
        StyleHelper::drawIconWithShadow(icon(), iconRect, &painter, iconMode);
        painter.setFont(normalFont);

        QPoint textOffset = centerRect.center()
                            - QPoint(iconRect.width() / 2, iconRect.height() / 2);
        textOffset = textOffset - QPoint(0, lineHeight + 3);
        const QRectF r(0, textOffset.y(), rect().width(), lineHeight);
        painter.setPen(creatorTheme()->color(isEnabled() ? Theme::PanelTextColorLight
                                                         : Theme::IconsDisabledColor));

        // draw project name
        const int margin = 6;
        const qreal availableWidth = r.width() - margin;
        const QString ellidedProjectName = fm.elidedText(projectName,
                                                         Qt::ElideMiddle,
                                                         int(availableWidth));
        painter.drawText(r, textFlags, ellidedProjectName);

        // draw build configuration name
        textOffset = iconRect.center() + QPoint(iconRect.width() / 2, iconRect.height() / 2);
        QRectF buildConfigRect[2];
        buildConfigRect[0] = QRectF(0, textOffset.y() + 4, rect().width(), lineHeight);
        buildConfigRect[1] = QRectF(0, textOffset.y() + 4 + lineHeight, rect().width(), lineHeight);
        painter.setFont(boldFont);
        QVector<QString> splitBuildConfiguration(2);
        const QString buildConfiguration = defaultAction()->property("subtitle").toString();
        if (boldFm.width(buildConfiguration) <= availableWidth)
            // text fits in one line
            splitBuildConfiguration[0] = buildConfiguration;
        else
            splitBuildConfiguration = splitInTwoLines(buildConfiguration, boldFm, availableWidth);

        // draw the two text lines for the build configuration
        painter.setPen(
            creatorTheme()->color(isEnabled()
                                      // Intentionally using the "Unselected" colors,
                                      // because the text color won't change in the pressed
                                      // state as they would do on the mode buttons.
                                      ? Theme::FancyTabWidgetEnabledUnselectedTextColor
                                      : Theme::FancyTabWidgetDisabledUnselectedTextColor));

        for (int i = 0; i < 2; ++i) {
            const QString &buildConfigText = splitBuildConfiguration[i];
            if (buildConfigText.isEmpty())
                continue;
            painter.drawText(buildConfigRect[i], textFlags, buildConfigText);
        }

    } else {
        iconRect.moveCenter(rect().center());
        StyleHelper::drawIconWithShadow(icon(), iconRect, &painter, iconMode);
    }

    // pop up arrow next to icon
    if (isTitledAction && isEnabled() && !icon().isNull()) {
        QStyleOption opt;
        opt.initFrom(this);
        opt.rect = rect().adjusted(rect().width() -
                                  (m_iconsOnly ? 6 : 16), 0, -(m_iconsOnly ? 0 : 8), 0);
        StyleHelper::drawArrow(QStyle::PE_IndicatorArrowRight, &painter, &opt);
    }
}

void FancyActionBar::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    const QRectF borderRect = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    if (creatorTheme()->flag(Theme::FlatToolBars)) {
        // this paints the background of the bottom portion of the
        // left tab bar
        painter.fillRect(event->rect(), StyleHelper::baseColor());
        painter.setPen(creatorTheme()->color(Theme::FancyToolBarSeparatorColor));
        painter.drawLine(borderRect.topLeft(), borderRect.topRight());
    } else {
        painter.setPen(StyleHelper::sidebarShadow());
        painter.drawLine(borderRect.topLeft(), borderRect.topRight());
        painter.setPen(StyleHelper::sidebarHighlight());
        painter.drawLine(borderRect.topLeft() + QPointF(1, 1),
                         borderRect.topRight() + QPointF(0, 1));
    }
}

QSize FancyToolButton::sizeHint() const
{
    if (m_iconsOnly) {
        return {Core::Constants::MODEBAR_ICONSONLY_BUTTON_SIZE,
                Core::Constants::MODEBAR_ICONSONLY_BUTTON_SIZE};
    }

    QSizeF buttonSize = iconSize().expandedTo(QSize(64, 38));
    if (defaultAction()->property("titledAction").toBool()) {
        QFont boldFont(font());
        boldFont.setPointSizeF(StyleHelper::sidebarFontSize());
        boldFont.setBold(true);
        const QFontMetrics fm(boldFont);
        const qreal lineHeight = fm.height();
        const QString projectName = defaultAction()->property("heading").toString();
        buttonSize += QSizeF(0, 10);
        if (!projectName.isEmpty())
            buttonSize += QSizeF(0, lineHeight + 2);

        buttonSize += QSizeF(0, lineHeight * 2 + 2);
    }
    return buttonSize.toSize();
}

QSize FancyToolButton::minimumSizeHint() const
{
    return {8, 8};
}

void FancyToolButton::setIconsOnly(bool iconsOnly)
{
    m_iconsOnly = iconsOnly;
    updateGeometry();
}

void FancyToolButton::hoverOverlay(QPainter *painter, const QRect &spanRect)
{
    const QSize logicalSize = spanRect.size();
    const QString cacheKey = QLatin1String(Q_FUNC_INFO) + QString::number(logicalSize.width())
                             + QLatin1Char('x') + QString::number(logicalSize.height());
    QPixmap overlay;
    if (!QPixmapCache::find(cacheKey, &overlay)) {
        const int dpr = painter->device()->devicePixelRatio();
        overlay = QPixmap(logicalSize * dpr);
        overlay.fill(Qt::transparent);
        overlay.setDevicePixelRatio(dpr);

        const QColor hoverColor = creatorTheme()->color(Theme::FancyToolButtonHoverColor);
        const QRect rect(QPoint(), logicalSize);
        const QRectF borderRect = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);

        QLinearGradient grad(rect.topLeft(), rect.topRight());
        grad.setColorAt(0, Qt::transparent);
        grad.setColorAt(0.5, hoverColor);
        grad.setColorAt(1, Qt::transparent);

        QPainter p(&overlay);
        p.fillRect(rect, grad);
        p.setPen(QPen(grad, 1.0));
        p.drawLine(borderRect.topLeft(), borderRect.topRight());
        p.drawLine(borderRect.bottomLeft(), borderRect.bottomRight());
        p.end();

        QPixmapCache::insert(cacheKey, overlay);
    }
    painter->drawPixmap(spanRect.topLeft(), overlay);
}

void FancyToolButton::actionChanged()
{
    // the default action changed in some way, e.g. it might got hidden
    // since we inherit a tool button we won't get invisible, so do this here
    if (QAction *action = defaultAction())
        setVisible(action->isVisible());
}

FancyActionBar::FancyActionBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("actionbar");
    m_actionsLayout = new QVBoxLayout;
    m_actionsLayout->setMargin(0);
    m_actionsLayout->setSpacing(0);
    setLayout(m_actionsLayout);
    setContentsMargins(0, 2, 0, 8);
}

void FancyActionBar::addProjectSelector(QAction *action)
{
    insertAction(0, action);
}

void FancyActionBar::insertAction(int index, QAction *action)
{
    auto *button = new FancyToolButton(action, this);
    button->setIconsOnly(m_iconsOnly);
    m_actionsLayout->insertWidget(index, button);
}

QLayout *FancyActionBar::actionsLayout() const
{
    return m_actionsLayout;
}

QSize FancyActionBar::minimumSizeHint() const
{
    return sizeHint();
}

void FancyActionBar::setIconsOnly(bool iconsOnly)
{
    m_iconsOnly = iconsOnly;
    for (int i = 0, c = m_actionsLayout->count(); i < c; ++i) {
        if (auto *button = qobject_cast<FancyToolButton*>(m_actionsLayout->itemAt(i)->widget()))
            button->setIconsOnly(iconsOnly);
    }
    setContentsMargins(0, iconsOnly ? 7 : 2, 0, iconsOnly ? 2 : 8);
}

} // namespace Internal
} // namespace Core
