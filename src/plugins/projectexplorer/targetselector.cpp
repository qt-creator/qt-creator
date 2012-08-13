/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "targetselector.h"

#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <QPainter>
#include <QMenu>
#include <QMouseEvent>
#include <QFontMetrics>

static const int TARGET_HEIGHT = 43;
static const int NAVBUTTON_WIDTH = 27;

using namespace ProjectExplorer::Internal;

TargetSelector::TargetSelector(QWidget *parent) :
    QWidget(parent),
    m_unselected(QLatin1String(":/projectexplorer/images/targetunselected.png")),
    m_runselected(QLatin1String(":/projectexplorer/images/targetrunselected.png")),
    m_buildselected(QLatin1String(":/projectexplorer/images/targetbuildselected.png")),
    m_targetRightButton(QLatin1String(":/projectexplorer/images/targetrightbutton.png")),
    m_targetLeftButton(QLatin1String(":/projectexplorer/images/targetleftbutton.png")),
    m_targetRemoveButton(QLatin1String(":/projectexplorer/images/targetremovebutton.png")),
    m_targetRemoveDarkButton(QLatin1String(":/projectexplorer/images/targetremovebuttondark.png")),
    m_currentTargetIndex(-1),
    m_currentHoveredTargetIndex(-1),
    m_startIndex(0)
{
    QFont f = font();
    f.setPixelSize(10);
    f.setBold(true);
    setFont(f);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void TargetSelector::insertTarget(int index, const QString &name)
{
    QTC_ASSERT(index >= 0 && index <= m_targets.count(), return);

    Target target;
    target.name = name;
    target.currentSubIndex = 0;

    m_targets.insert(index, target);

    if (m_currentTargetIndex >= index)
        setCurrentIndex(m_currentTargetIndex + 1);
    updateGeometry();
    update();
}

void TargetSelector::renameTarget(int index, const QString &name)
{
    m_targets[index].name = name;
    update();
}

void TargetSelector::removeTarget(int index)
{
    QTC_ASSERT(index >= 0 && index < m_targets.count(), return);

    m_targets.removeAt(index);

    if (m_currentTargetIndex > index) {
        --m_currentTargetIndex;
        // force a signal since the index has changed
        emit currentChanged(m_currentTargetIndex, m_targets.at(m_currentTargetIndex).currentSubIndex);
    }
    updateGeometry();
    update();
}

void TargetSelector::setCurrentIndex(int index)
{
    if (index < -1 ||
        index >= m_targets.count() ||
        index == m_currentTargetIndex)
        return;

    if (index == -1 && !m_targets.isEmpty())
        return;

    m_currentTargetIndex = index;

    update();
    emit currentChanged(m_currentTargetIndex,
                             m_currentTargetIndex >= 0 ? m_targets.at(m_currentTargetIndex).currentSubIndex : -1);
}

void TargetSelector::setCurrentSubIndex(int subindex)
{
    if (subindex < 0 ||
        subindex >= 2 ||
        m_currentTargetIndex < 0 ||
        subindex == m_targets.at(m_currentTargetIndex).currentSubIndex)
        return;
    m_targets[m_currentTargetIndex].currentSubIndex = subindex;

    update();
    emit currentChanged(m_currentTargetIndex,
                             m_targets.at(m_currentTargetIndex).currentSubIndex);
}

TargetSelector::Target TargetSelector::targetAt(int index) const
{
    return m_targets.at(index);
}

int TargetSelector::targetWidth() const
{
    static int width = -1;
    if (width < 0) {
        QFontMetrics fm = fontMetrics();
        width = qMax(fm.width(runButtonString()), fm.width(buildButtonString()));
        width = qMax(149, width * 2 + 31);
    }
    return width;
}

QSize TargetSelector::sizeHint() const
{
    return QSize((targetWidth() + 1) * m_targets.size() + (NAVBUTTON_WIDTH + 1) * 2 + 3, TARGET_HEIGHT + 1);
}

int TargetSelector::maxVisibleTargets() const
{
    return (width() - ((NAVBUTTON_WIDTH + 1) * 2 + 3))/(targetWidth() + 1);
}

void TargetSelector::getControlAt(int x, int y, int *buttonIndex, int *targetIndex, int *targetSubIndex, bool *removeButton)
{
    if (buttonIndex)
        *buttonIndex = -1;
    if (targetIndex)
        *targetIndex = -1;
    if (targetSubIndex)
        *targetSubIndex = -1;
    if (removeButton)
        *removeButton = false;

    // left button?
    if (m_startIndex > 0 /* button visible */ && x >= 0 && x < NAVBUTTON_WIDTH + 2) {
        if (buttonIndex)
            *buttonIndex = 0;
        return;
    }

    // right button?
    int rightButtonStartX = NAVBUTTON_WIDTH + (targetWidth() + 1) * maxVisibleTargets() + 2;
    if (x > rightButtonStartX) {
        if (m_targets.size() > maxVisibleTargets() /* button visible */ && x <= rightButtonStartX + NAVBUTTON_WIDTH + 1) {
            if (buttonIndex)
                *buttonIndex = 1;
        }
        return;
    }

    // find the clicked target button
    int tx = NAVBUTTON_WIDTH + 3;
    int index;
    for (index = m_startIndex; index < m_targets.size(); ++index) {
        if (x <= tx) {
            break;
        }
        tx += targetWidth() + 1;
    }
    --index;
    tx -= targetWidth() + 1;
    if (index >= 0 && index < m_targets.size()) {
        if (targetIndex)
            *targetIndex = index;
        // handle clicked target
        // check if user clicked on Build or Run
        if (y > TARGET_HEIGHT * 3/5) {
            if ((x - tx) - 2 > targetWidth() / 2) {
                if (targetSubIndex)
                    *targetSubIndex = 1;
            } else {
                if (targetSubIndex)
                    *targetSubIndex = 0;
            }
        } else if (y < m_targetRemoveButton.height() + 3
                   && x >= tx + targetWidth() - m_targetRemoveButton.width() - 1) {
            if (removeButton)
                *removeButton = true;
        }
    }
}

void TargetSelector::mousePressEvent(QMouseEvent *event)
{
    int buttonIndex;
    int targetIndex;
    int targetSubIndex;
    bool removeButton;
    getControlAt(event->x(), event->y(), &buttonIndex, &targetIndex, &targetSubIndex, &removeButton);
    if (buttonIndex == 0) {
        event->accept();
        --m_startIndex;
        update();
    } else if (buttonIndex == 1) {
        event->accept();
        ++m_startIndex;
        update();
    } else if (targetIndex != -1) {
        event->accept();
        bool updateNeeded = false;
        if (targetIndex != m_currentTargetIndex && !removeButton) {
            m_currentTargetIndex = targetIndex;
            updateNeeded = true;
        }
        if (targetSubIndex != -1) {
            if (targetSubIndex != m_targets[m_currentTargetIndex].currentSubIndex) {
                m_targets[m_currentTargetIndex].currentSubIndex = targetSubIndex;
                updateNeeded = true;
            }
        } else if (removeButton) {
            emit removeButtonClicked(targetIndex);
        }
        if (updateNeeded) {
            update();
            emit currentChanged(m_currentTargetIndex, m_targets.at(m_currentTargetIndex).currentSubIndex);
        }
    } else {
        event->ignore();
    }
}

void TargetSelector::mouseMoveEvent(QMouseEvent *event)
{
    int targetIndex;
    getControlAt(event->x(), event->y(), 0, &targetIndex, 0, 0);
    if (m_currentHoveredTargetIndex != targetIndex) {
        m_currentHoveredTargetIndex = targetIndex;
        if (targetIndex != -1)
            event->accept();
        update();
    }
}

void TargetSelector::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_currentHoveredTargetIndex = -1;
    update();
}

void TargetSelector::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // update start index depending on available width
    m_startIndex = qMax(0, qMin(m_startIndex, m_targets.size() - maxVisibleTargets()));

    QPainter p(this);
    QColor borderColor(89, 89, 89);

    int x = 2;
    QFontMetrics fm(font());

    //draw left button
    if (m_startIndex > 0)
        p.drawPixmap(x, 1, m_targetLeftButton);
    x += m_targetLeftButton.width();
    if (m_startIndex == 0) {
        p.setPen(borderColor);
        p.drawLine(x, 1, x, TARGET_HEIGHT);
    }
    x += 1;
    // draw targets
    const QString runString = runButtonString();
    const QString buildString = buildButtonString();
    const int lastIndex = qMin(m_targets.size(), m_startIndex + maxVisibleTargets()) - 1;
    for (int index = m_startIndex; index <= lastIndex; ++index) {
        const Target &target = m_targets.at(index);
        QImage image = m_unselected;
        bool buildSelected = target.currentSubIndex == 0;
        if (index == m_currentTargetIndex) {
            p.setPen(QColor(255, 255, 255));
            if (buildSelected) {
                image = m_buildselected;
            } else {
                image = m_runselected;
            }
        } else {
            p.setPen(Qt::black);
        }

        QRect buttonRect(x, 1, targetWidth() , image.height());
        Utils::StyleHelper::drawCornerImage(image, &p, buttonRect, 16, 0, 16, 0);
        const QString nameText = QFontMetrics(font()).elidedText(target.name, Qt::ElideRight,
                                                                 targetWidth() - 6);
        p.drawText(x + (targetWidth()- fm.width(nameText))/2 + 1, 7 + fm.ascent(),
            nameText);

        // remove button
        if (m_currentHoveredTargetIndex == index) {
            p.drawPixmap(x + targetWidth() - m_targetRemoveButton.width() - 2, 3,
                         index == m_currentTargetIndex ? m_targetRemoveDarkButton : m_targetRemoveButton);
        }

        // Build
        int margin = 2; // position centered within the rounded buttons
        QFontMetrics fm = fontMetrics();
        QRect textRect(x + margin, size().height() - fm.height() - 5, targetWidth()/2, fm.height());
        if (index != m_currentTargetIndex)
            p.setPen(QColor(0x555555));
        else
            p.setPen(buildSelected ? Qt::black : Qt::white);

        p.drawText(textRect, Qt::AlignHCenter, buildString);

        // Run
        textRect.moveLeft(x + targetWidth()/2 - 2 * margin);
        if (index != m_currentTargetIndex)
            p.setPen(QColor(0x555555));
        else
            p.setPen(buildSelected ? Qt::white: Qt::black);
        p.drawText(textRect, Qt::AlignHCenter, runString);

        x += targetWidth();

        p.setPen(index == m_currentTargetIndex ? QColor(0x222222) : QColor(0xcccccc));
        p.drawLine(x, 1, x, TARGET_HEIGHT);
        ++x;
    }
    // draw right button and frame (left hand part already done)
    p.setPen(borderColor);
    p.drawLine(2 + m_targetLeftButton.width(), 0, x - 1, 0);
    if (lastIndex < m_targets.size() - 1)
        p.drawPixmap(x, 1, m_targetRightButton);
    else
        p.drawLine(x - 1, 1, x - 1, TARGET_HEIGHT);
}
