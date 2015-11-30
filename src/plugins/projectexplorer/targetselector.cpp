/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "targetselector.h"

#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <QPainter>
#include <QMenu>
#include <QMouseEvent>
#include <QFontMetrics>
#include <QPushButton>

static const int TARGET_HEIGHT = 43;
static const int NAVBUTTON_WIDTH = 27;
static const int KITNAME_MARGINS = 6;

namespace ProjectExplorer {
namespace Internal {
class QPixmapButton : public QPushButton
{
public:
    QPixmapButton(QWidget *parent, const QPixmap &first, const QPixmap &second)
        : QPushButton(parent), m_showFirst(true), m_first(first), m_second(second)
    {
        setFixedSize(m_first.size());
    }

    void paintEvent(QPaintEvent *)
    {
        QPainter p(this);
        p.drawPixmap(0, 0, m_showFirst ? m_first : m_second);
    }

    void setFirst(bool f)
    {
        m_showFirst = f;
    }

private:
    bool m_showFirst;
    const QPixmap m_first;
    const QPixmap m_second;
};
}
}

using namespace ProjectExplorer::Internal;

TargetSelector::TargetSelector(QWidget *parent) :
    QWidget(parent),
    m_unselected(QLatin1String(":/projectexplorer/images/targetunselected.png")),
    m_runselected(Utils::StyleHelper::dpiSpecificImageFile(QLatin1String(":/projectexplorer/images/targetrunselected.png"))),
    m_buildselected(m_runselected.mirrored(true, false)),
    m_targetRightButton(Utils::StyleHelper::dpiSpecificImageFile(QLatin1String(":/projectexplorer/images/targetrightbutton.png"))),
    m_targetLeftButton(QPixmap::fromImage(m_targetRightButton.toImage().mirrored(true, false))),
    m_targetChangePixmap(Utils::StyleHelper::dpiSpecificImageFile(QLatin1String(":/projectexplorer/images/targetchangebutton.png"))),
    m_targetChangePixmap2(Utils::StyleHelper::dpiSpecificImageFile(QLatin1String(":/projectexplorer/images/targetchangebutton2.png"))),
    m_currentTargetIndex(-1),
    m_currentHoveredTargetIndex(-1),
    m_startIndex(0),
    m_menuShown(false),
    m_targetWidthNeedsUpdate(true)
{
    QFont f = font();
    f.setPixelSize(10);
    f.setBold(true);
    setFont(f);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_targetChangeButton = new QPixmapButton(this, m_targetChangePixmap2, m_targetChangePixmap);
    m_targetChangeButton->hide();
    connect(m_targetChangeButton, SIGNAL(pressed()), this, SLOT(changeButtonPressed()));
}

void TargetSelector::changeButtonPressed()
{
    emit menuShown(m_currentHoveredTargetIndex);
}

void TargetSelector::menuAboutToShow()
{
    m_menuShown = true;
    updateButtons();
}

void TargetSelector::menuAboutToHide()
{
    m_menuShown = false;
    updateButtons();
}

void TargetSelector::insertTarget(int index, int subIndex, const QString &name)
{
    QTC_ASSERT(index >= 0 && index <= m_targets.count(), return);

    Target target;
    target.name = name;
    target.currentSubIndex = subIndex;

    m_targets.insert(index, target);

    if (m_currentTargetIndex >= index)
        setCurrentIndex(m_currentTargetIndex + 1);
    m_targetWidthNeedsUpdate = true;
    updateGeometry();
    update();
}

void TargetSelector::renameTarget(int index, const QString &name)
{
    m_targets[index].name = name;
    m_targetWidthNeedsUpdate = true;
    updateGeometry();
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
    m_targetWidthNeedsUpdate = true;
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

    if (isVisible())
        ensureCurrentIndexVisible();

    update();
    emit currentChanged(m_currentTargetIndex,
                             m_currentTargetIndex >= 0 ? m_targets.at(m_currentTargetIndex).currentSubIndex : -1);
}

void TargetSelector::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    ensureCurrentIndexVisible();
}

void TargetSelector::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    ensureCurrentIndexVisible();
}

void TargetSelector::ensureCurrentIndexVisible()
{
    if (m_currentTargetIndex < m_startIndex)
        m_startIndex = m_currentTargetIndex;
    const int lastIndex = m_startIndex + maxVisibleTargets() - 1;
    if (m_currentTargetIndex > lastIndex)
        m_startIndex = m_currentTargetIndex - maxVisibleTargets() + 1;
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

void TargetSelector::setTargetMenu(QMenu *menu)
{
    if (m_targetChangeButton->menu()) {
        disconnect(m_targetChangeButton->menu(), SIGNAL(aboutToShow()),
                   this, SLOT(menuAboutToShow()));
        disconnect(m_targetChangeButton->menu(), SIGNAL(aboutToHide()),
                   this, SLOT(menuAboutToHide()));
    }

    m_targetChangeButton->setMenu(menu);

    if (menu) {
        connect(m_targetChangeButton->menu(), SIGNAL(aboutToShow()),
                this, SLOT(menuAboutToShow()));
        connect(m_targetChangeButton->menu(), SIGNAL(aboutToHide()),
                this, SLOT(menuAboutToHide()));
    }
}

int TargetSelector::targetWidth() const
{
    static int width = -1;
    if (width < 0 || m_targetWidthNeedsUpdate) {
        m_targetWidthNeedsUpdate = false;
        QFontMetrics fm = fontMetrics();
        width = 149; // minimum
        // let it grow for the kit names ...
        foreach (const Target &target, m_targets)
            width = qMax(width, fm.width(target.name) + KITNAME_MARGINS + 2/*safety measure*/);
        width = qMin(width, 299); // ... but not too much
        int buttonWidth = qMax(fm.width(runButtonString()), fm.width(buildButtonString()));
        width = qMax(width, buttonWidth * 2 + 31); // run & build button strings must be fully visible
    }
    return width;
}

QSize TargetSelector::sizeHint() const
{
    return QSize((targetWidth() + 1) * m_targets.size() + (NAVBUTTON_WIDTH + 1) * 2 + 3, TARGET_HEIGHT + 1);
}

int TargetSelector::maxVisibleTargets() const
{
    return qMax((width() - ((NAVBUTTON_WIDTH + 1) * 2 + 3))/(targetWidth() + 1), 1);
}

void TargetSelector::getControlAt(int x, int y, int *buttonIndex, int *targetIndex, int *targetSubIndex)
{
    if (buttonIndex)
        *buttonIndex = -1;
    if (targetIndex)
        *targetIndex = -1;
    if (targetSubIndex)
        *targetSubIndex = -1;

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
        if (x <= tx)
            break;
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
        }
    }
}

void TargetSelector::mousePressEvent(QMouseEvent *event)
{
    int buttonIndex;
    int targetIndex;
    int targetSubIndex;
    getControlAt(event->x(), event->y(), &buttonIndex, &targetIndex, &targetSubIndex);
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
        if (targetIndex != m_currentTargetIndex) {
            m_currentTargetIndex = targetIndex;
            updateNeeded = true;
        }
        if (targetSubIndex != -1) {
            if (targetSubIndex != m_targets[m_currentTargetIndex].currentSubIndex) {
                m_targets[m_currentTargetIndex].currentSubIndex = targetSubIndex;
                updateNeeded = true;
            }
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
    getControlAt(event->x(), event->y(), 0, &targetIndex, 0);
    if (m_currentHoveredTargetIndex != targetIndex) {
        m_currentHoveredTargetIndex = targetIndex;
        if (targetIndex != -1)
            event->accept();
        updateButtons();
        update();
    }
}

void TargetSelector::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    m_currentHoveredTargetIndex = -1;
    updateButtons();
    update();
}

void TargetSelector::updateButtons()
{
    if (m_menuShown) {
        // Do nothing while the menu is show
    } else if (m_currentHoveredTargetIndex == -1) {
        m_targetChangeButton->hide();
    } else {
        int tx = NAVBUTTON_WIDTH + 3 + (m_currentHoveredTargetIndex - m_startIndex) * (targetWidth() + 1);

        const int pixmapWidth =
                static_cast<int>(m_targetChangePixmap.width() / m_targetChangePixmap.devicePixelRatio());
        const QPoint buttonTopLeft(tx + targetWidth() - pixmapWidth - 1, 3);
        m_targetChangeButton->move(buttonTopLeft);
        m_targetChangeButton->setVisible(true);
        m_targetChangeButton->setFirst(m_currentHoveredTargetIndex == m_currentTargetIndex);
    }
}

bool TargetSelector::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        const QHelpEvent *helpEvent = static_cast<const QHelpEvent *>(e);
        int targetIndex;
        int subTargetIndex;
        getControlAt(helpEvent->x(), helpEvent->y(), 0, &targetIndex, &subTargetIndex);
        if (targetIndex >= 0 && subTargetIndex < 0) {
            emit toolTipRequested(helpEvent->globalPos(), targetIndex);
            e->accept();
            return true;
        }
    }
    return QWidget::event(e);
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
    x += static_cast<int>(m_targetLeftButton.width() / m_targetLeftButton.devicePixelRatio());
    if (m_startIndex == 0) {
        p.setPen(borderColor);
        p.drawLine(QLineF(x + 0.5, 1.5, x + 0.5, TARGET_HEIGHT + 0.5));
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
            if (buildSelected)
                image = m_buildselected;
            else
                image = m_runselected;
        } else {
            p.setPen(Qt::black);
        }

        QRect buttonRect(x, 1, targetWidth(), static_cast<int>(image.height() / image.devicePixelRatio()));
        Utils::StyleHelper::drawCornerImage(image, &p, buttonRect, 13, 0, 13, 0);
        const QString nameText = QFontMetrics(font()).elidedText(target.name, Qt::ElideRight,
                                                                 targetWidth() - KITNAME_MARGINS);
        p.drawText(x + (targetWidth()- fm.width(nameText))/2 + 1, 7 + fm.ascent(),
            nameText);

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
        p.drawLine(QLineF(x + 0.5, 1.5, x + 0.5, TARGET_HEIGHT + 0.5));
        ++x;
    }
    // draw right button and frame (left hand part already done)
    p.setPen(borderColor);
    p.drawLine(QLineF(2.5 + m_targetLeftButton.width() / m_targetLeftButton.devicePixelRatio(),
               0.5, x - 0.5, 0.5));
    if (lastIndex < m_targets.size() - 1)
        p.drawPixmap(x, 1, m_targetRightButton);
    else
        p.drawLine(QLineF(x - 0.5, 1.5, x - 0.5, TARGET_HEIGHT + 0.5));
}
