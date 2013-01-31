/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "fancyactionbar.h"
#include "coreconstants.h"

#include <utils/stylehelper.h>
#include <utils/stringutils.h>
#include <utils/tooltip/tooltip.h>
#include <utils/tooltip/tipcontents.h>

#include <coreplugin/icore.h>
#include <coreplugin/imode.h>

#include <QHBoxLayout>
#include <QPainter>
#include <QPicture>
#include <QVBoxLayout>
#include <QAction>
#include <QStatusBar>
#include <QStyle>
#include <QStyleOption>
#include <QMouseEvent>
#include <QApplication>
#include <QEvent>
#include <QAnimationGroup>
#include <QPropertyAnimation>
#include <QDebug>

using namespace Core;
using namespace Internal;

FancyToolButton::FancyToolButton(QWidget *parent)
    : QToolButton(parent), m_fader(0)
{
    m_hasForceVisible = false;
    setAttribute(Qt::WA_Hover, true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void FancyToolButton::forceVisible(bool visible)
{
    m_hasForceVisible = true;
    setVisible(visible);
}

bool FancyToolButton::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::Enter:
        {
            QPropertyAnimation *animation = new QPropertyAnimation(this, "fader");
            animation->setDuration(125);
            animation->setEndValue(1.0);
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }
        break;
    case QEvent::Leave:
        {
            QPropertyAnimation *animation = new QPropertyAnimation(this, "fader");
            animation->setDuration(125);
            animation->setEndValue(0.0);
            animation->start(QAbstractAnimation::DeleteWhenStopped);
        }
        break;
    case QEvent::ToolTip:
        {
            QHelpEvent *he = static_cast<QHelpEvent *>(e);
            Utils::ToolTip::instance()->show(mapToGlobal(he->pos()), Utils::TextContent(toolTip()), this);
            return true;
        }
    default:
        return QToolButton::event(e);
    }
    return false;
}

static QVector<QString> splitInTwoLines(const QString &text, const QFontMetrics &fontMetrics,
                                        qreal availableWidth)
{
    // split in two lines.
    // this looks if full words can be split off at the end of the string,
    // to put them in the second line. First line is drawn with ellipsis,
    // second line gets ellipsis if it couldn't split off full words.
    QVector<QString> splitLines(2);
    QRegExp rx(QLatin1String("\\s+"));
    int splitPos = -1;
    int nextSplitPos = text.length();
    do {
        nextSplitPos = rx.lastIndexIn(text,
                                      nextSplitPos - text.length() - 1);
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
        splitLines[0] = fontMetrics.elidedText(text, Qt::ElideRight,
                                                       availableWidth);
        QString common = Utils::commonPrefix(QStringList()
                                             << splitLines[0] << text);
        splitLines[1] = text.mid(common.length());
        // elide the second line even if it fits, since it is cut off in mid-word
        while (fontMetrics.width(QChar(0x2026) /*'...'*/ + splitLines[1]) > availableWidth
               && splitLines[1].length() > 3
               /*keep at least three original characters (should not happen)*/) {
            splitLines[1].remove(0, 1);
        }
        splitLines[1] = QChar(0x2026) /*'...'*/ + splitLines[1];
    } else {
        splitLines[0] = fontMetrics.elidedText(text.left(splitPos).trimmed(), Qt::ElideRight, availableWidth);
        splitLines[1] = text.mid(splitPos);
    }
    return splitLines;
}

void FancyToolButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);

    // draw borders
    bool isTitledAction = defaultAction()->property("titledAction").toBool();

#ifndef Q_OS_MAC // Mac UIs usually don't hover
    if (m_fader > 0 && isEnabled() && !isDown() && !isChecked()) {
        painter.save();
        int fader = int(40 * m_fader);
        QLinearGradient grad(rect().topLeft(), rect().topRight());
        grad.setColorAt(0, Qt::transparent);
        grad.setColorAt(0.5, QColor(255, 255, 255, fader));
        grad.setColorAt(1, Qt::transparent);
        painter.fillRect(rect(), grad);
        painter.setPen(QPen(grad, 1.0));
        painter.drawLine(rect().topLeft(), rect().topRight());
        painter.drawLine(rect().bottomLeft(), rect().bottomRight());
        painter.restore();
    } else
#endif
    if (isDown() || isChecked()) {
        painter.save();
        QLinearGradient grad(rect().topLeft(), rect().topRight());
        grad.setColorAt(0, Qt::transparent);
        grad.setColorAt(0.5, QColor(0, 0, 0, 50));
        grad.setColorAt(1, Qt::transparent);
        painter.fillRect(rect(), grad);
        painter.setPen(QPen(grad, 1.0));
        painter.drawLine(rect().topLeft(), rect().topRight());
        painter.drawLine(rect().topLeft(), rect().topRight());
        painter.drawLine(rect().topLeft() + QPoint(0,1), rect().topRight() + QPoint(0,1));
        painter.drawLine(rect().bottomLeft(), rect().bottomRight());
        painter.drawLine(rect().bottomLeft(), rect().bottomRight());
        painter.drawLine(rect().topLeft() - QPoint(0,1), rect().topRight() - QPoint(0,1));
        painter.restore();
    }
    QPixmap borderPixmap;
    QMargins margins;

    QRect iconRect(0, 0, Core::Constants::TARGET_ICON_SIZE, Core::Constants::TARGET_ICON_SIZE);
    // draw popup texts
    if (isTitledAction) {

        QFont normalFont(painter.font());
        QRect centerRect = rect();
        normalFont.setPointSizeF(Utils::StyleHelper::sidebarFontSize());
        QFont boldFont(normalFont);
        boldFont.setBold(true);
        QFontMetrics fm(normalFont);
        QFontMetrics boldFm(boldFont);
        int lineHeight = boldFm.height();
        int textFlags = Qt::AlignVCenter|Qt::AlignHCenter;

        const QString projectName = defaultAction()->property("heading").toString();
        if (!projectName.isNull())
            centerRect.adjust(0, lineHeight + 4, 0, 0);

        centerRect.adjust(0, 0, 0, -lineHeight*2 - 4);

        iconRect.moveCenter(centerRect.center());
        Utils::StyleHelper::drawIconWithShadow(icon(), iconRect, &painter, isEnabled() ? QIcon::Normal : QIcon::Disabled);
        painter.setFont(normalFont);

        QPoint textOffset = centerRect.center() - QPoint(iconRect.width()/2, iconRect.height()/2);
        textOffset = textOffset - QPoint(0, lineHeight + 4);
        QRectF r(0, textOffset.y(), rect().width(), lineHeight);
        QColor penColor;
        if (isEnabled())
            penColor = Qt::white;
        else
            penColor = Qt::gray;
        painter.setPen(penColor);

        // draw project name
        const int margin = 6;
        const qreal availableWidth = r.width() - margin;
        QString ellidedProjectName = fm.elidedText(projectName, Qt::ElideMiddle, availableWidth);
        if (isEnabled()) {
            const QRectF shadowR = r.translated(0, 1);
            painter.setPen(QColor(30, 30, 30, 80));
            painter.drawText(shadowR, textFlags, ellidedProjectName);
            painter.setPen(penColor);
        }
        painter.drawText(r, textFlags, ellidedProjectName);

        // draw build configuration name
        textOffset = iconRect.center() + QPoint(iconRect.width()/2, iconRect.height()/2);
        QRectF buildConfigRect[2];
        buildConfigRect[0] = QRectF(0, textOffset.y() + 5, rect().width(), lineHeight);
        buildConfigRect[1] = QRectF(0, textOffset.y() + 5 + lineHeight, rect().width(), lineHeight);
        painter.setFont(boldFont);
        QVector<QString> splitBuildConfiguration(2);
        const QString buildConfiguration = defaultAction()->property("subtitle").toString();
        if (boldFm.width(buildConfiguration) <= availableWidth) {
            // text fits in one line
            splitBuildConfiguration[0] = buildConfiguration;
        } else {
            splitBuildConfiguration = splitInTwoLines(buildConfiguration, boldFm, availableWidth);
        }
        // draw the two lines for the build configuration
        for (int i = 0; i < 2; ++i) {
            if (splitBuildConfiguration[i].isEmpty())
                continue;
            if (isEnabled()) {
                const QRectF shadowR = buildConfigRect[i].translated(0, 1);
                painter.setPen(QColor(30, 30, 30, 80));
                painter.drawText(shadowR, textFlags, splitBuildConfiguration[i]);
                painter.setPen(penColor);
            }
            painter.drawText(buildConfigRect[i], textFlags, splitBuildConfiguration[i]);
        }

        // pop up arrow next to icon
        if (!icon().isNull()) {
            QStyleOption opt;
            opt.initFrom(this);
            opt.rect = rect().adjusted(rect().width() - 16, 0, -8, 0);
            Utils::StyleHelper::drawArrow(QStyle::PE_IndicatorArrowRight, &painter, &opt);
        }
    } else {
        iconRect.moveCenter(rect().center());
        Utils::StyleHelper::drawIconWithShadow(icon(), iconRect, &painter, isEnabled() ? QIcon::Normal : QIcon::Disabled);
    }
}

void FancyActionBar::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    Q_UNUSED(event)
    QColor light = Utils::StyleHelper::sidebarHighlight();
    QColor dark = Utils::StyleHelper::sidebarShadow();
    painter.setPen(dark);
    painter.drawLine(rect().topLeft(), rect().topRight());
    painter.setPen(light);
    painter.drawLine(rect().topLeft() + QPoint(1,1), rect().topRight() + QPoint(0,1));
}

QSize FancyToolButton::sizeHint() const
{
    QSizeF buttonSize = iconSize().expandedTo(QSize(64, 38));
    if (defaultAction()->property("titledAction").toBool()) {
        QFont boldFont(font());
        boldFont.setPointSizeF(Utils::StyleHelper::sidebarFontSize());
        boldFont.setBold(true);
        QFontMetrics fm(boldFont);
        qreal lineHeight = fm.height();
        const QString projectName = defaultAction()->property("heading").toString();
        buttonSize += QSizeF(0, 10);
        if (!projectName.isEmpty())
            buttonSize += QSizeF(0, lineHeight + 2);

        buttonSize += QSizeF(0, lineHeight*2 + 2);
    }
    return buttonSize.toSize();
}

QSize FancyToolButton::minimumSizeHint() const
{
    return QSize(8, 8);
}

void FancyToolButton::actionChanged()
{
    // the default action changed in some way, e.g. it might got hidden
    // since we inherit a tool button we won't get invisible, so do this here
    if (!m_hasForceVisible) {
        if (QAction* action = defaultAction())
            setVisible(action->isVisible());
    }
}

FancyActionBar::FancyActionBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QLatin1String("actionbar"));
    m_actionsLayout = new QVBoxLayout;
    QVBoxLayout *spacerLayout = new QVBoxLayout;
    spacerLayout->addLayout(m_actionsLayout);
    int sbh = 8;
    spacerLayout->addSpacing(sbh);
    spacerLayout->setMargin(0);
    spacerLayout->setSpacing(0);
    setLayout(spacerLayout);
    setContentsMargins(0,2,0,0);
}

void FancyActionBar::addProjectSelector(QAction *action)
{
    FancyToolButton* toolButton = new FancyToolButton(this);
    toolButton->setDefaultAction(action);
    connect(action, SIGNAL(changed()), toolButton, SLOT(actionChanged()));
    m_actionsLayout->insertWidget(0, toolButton);

}
void FancyActionBar::insertAction(int index, QAction *action)
{
    FancyToolButton *toolButton = new FancyToolButton(this);
    toolButton->setDefaultAction(action);
    connect(action, SIGNAL(changed()), toolButton, SLOT(actionChanged()));
    m_actionsLayout->insertWidget(index, toolButton);
}

QLayout *FancyActionBar::actionsLayout() const
{
    return m_actionsLayout;
}

QSize FancyActionBar::minimumSizeHint() const
{
    return sizeHint();
}

