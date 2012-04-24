/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
static const int ADDBUTTON_WIDTH = 27;

using namespace ProjectExplorer::Internal;

TargetSelector::TargetSelector(QWidget *parent) :
    QWidget(parent),
    m_unselected(QLatin1String(":/projectexplorer/images/targetunselected.png")),
    m_runselected(QLatin1String(":/projectexplorer/images/targetrunselected.png")),
    m_buildselected(QLatin1String(":/projectexplorer/images/targetbuildselected.png")),
    m_targetaddbutton(QLatin1String(":/projectexplorer/images/targetaddbutton.png")),
    m_targetaddbuttondisabled(QLatin1String(":/projectexplorer/images/targetaddbutton_disabled.png")),
    m_targetremovebutton(QLatin1String(":/projectexplorer/images/targetremovebutton.png")),
    m_targetremovebuttondisabled(QLatin1String(":/projectexplorer/images/targetremovebutton_disabled.png")),
    m_currentTargetIndex(-1),
    m_addButtonEnabled(true),
    m_removeButtonEnabled(false),
    m_addButtonMenu(0)
{
    QFont f = font();
    f.setPixelSize(10);
    f.setBold(true);
    setFont(f);
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

void TargetSelector::setAddButtonEnabled(bool enabled)
{
    m_addButtonEnabled = enabled;
    update();
}

void TargetSelector::setRemoveButtonEnabled(bool enabled)
{
    m_removeButtonEnabled = enabled;
    update();
}

void TargetSelector::setAddButtonMenu(QMenu *menu)
{
    m_addButtonMenu = menu;
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

bool TargetSelector::isAddButtonEnabled() const
{
    return m_addButtonEnabled;
}

bool TargetSelector::isRemoveButtonEnabled() const
{
    return m_removeButtonEnabled;
}

int TargetSelector::targetWidth() const
{
    static int width = -1;
    if (width < 0) {
        QFontMetrics fm = fontMetrics();
        width = qMax(fm.width(runButtonString()), fm.width(buildButtonString()));
        width = qMax(129, width * 2 + 31);
    }
    return width;
}

QSize TargetSelector::minimumSizeHint() const
{
    return QSize((targetWidth() + 1) * m_targets.size() + (ADDBUTTON_WIDTH + 1) * 2 + 3, TARGET_HEIGHT + 4);
}

void TargetSelector::mousePressEvent(QMouseEvent *event)
{
    if (event->x() < ADDBUTTON_WIDTH) {
        event->accept();
        if (m_removeButtonEnabled)
            emit removeButtonClicked();
    } else if (event->x() > ADDBUTTON_WIDTH + (targetWidth() + 1) * m_targets.size()) {
        // check for add button
        event->accept();
        if (m_addButtonEnabled && m_addButtonMenu)
            m_addButtonMenu->popup(mapToGlobal(event->pos()));
    } else {
        // find the clicked target button
        int x = ADDBUTTON_WIDTH;
        int index;
        for (index = 0; index < m_targets.size(); ++index) {
            if (event->x() <= x) {
                break;
            }
            x += targetWidth() + 1;
        }
        --index;
        if (index >= 0 && index < m_targets.size()) {
            // handle clicked target
            // check if user clicked on Build or Run
            if (event->y() > TARGET_HEIGHT * 3/5) {
                if ((event->x() - (ADDBUTTON_WIDTH + (targetWidth() + 1) * index)) - 2 > targetWidth() / 2) {
                    m_targets[index].currentSubIndex = 1;
                } else {
                    m_targets[index].currentSubIndex = 0;
                }
            }
            m_currentTargetIndex = index;
            //TODO don't emit if nothing changed!
            update();
            emit currentChanged(m_currentTargetIndex, m_targets.at(m_currentTargetIndex).currentSubIndex);
        } else {
            event->ignore();
        }
    }
}

void TargetSelector::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.setPen(QColor(89, 89, 89));
    QSize size = minimumSizeHint();
    //draw frame
    p.drawLine(1, 0, size.width() - 2, 0);
    p.drawLine(1, size.height() - 3, size.width() - 2, size.height() - 3);
    p.drawLine(1, 1, 1, size.height() - 4);
    p.drawLine(size.width() - 2, 1, size.width() - 2, size.height() - 4);

    //draw shadow
    p.setPen(QColor(0, 0, 0, 50));
    p.drawLine(1, size.height() - 2, size.width() - 2, size.height() - 2);
    p.setPen(QColor(0, 0, 0, 20));
    p.drawLine(0, size.height() - 2, 0, size.height() - 9);
    p.drawLine(size.width()-1, size.height() - 2, size.width()-1, size.height() - 9);
    p.drawLine(1, size.height() - 1, size.width() - 2, size.height() - 1);

    //draw targets
    int x = 2;
    int index = 0;
    QFontMetrics fm(font());
    if (m_removeButtonEnabled)
        p.drawPixmap(x, 1, m_targetremovebutton);
    else
        p.drawPixmap(x, 1, m_targetremovebuttondisabled);
    x += m_targetremovebutton.width();
    p.setPen(QColor(0, 0, 0));
    p.drawLine(x, 1, x, TARGET_HEIGHT);
    x += 1;

    const QString runString = runButtonString();
    const QString buildString = buildButtonString();
    foreach (const Target &target, m_targets) {
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
        p.drawText(x + (targetWidth()- fm.width(target.name))/2 + 1, 7 + fm.ascent(),
            target.name);

        // Build
        int margin = 2; // position centered within the rounded buttons
        QFontMetrics fm = fontMetrics();
        QRect textRect(x + margin, size.height() - fm.height() - 7, targetWidth()/2, fm.height());
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
        ++index;
    }
    // draw add button
    if (m_addButtonEnabled)
        p.drawPixmap(x, 1, m_targetaddbutton);
    else
        p.drawPixmap(x, 1, m_targetaddbuttondisabled);
}
