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

#include "crumblepath.h"
#include "qtcassert.h"
#include "stylehelper.h"

#include <QList>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QStyle>
#include <QResizeEvent>
#include <QPainter>
#include <QImage>

namespace Utils {

static const int ArrowBorderSize = 12;

class CrumblePathButton : public QPushButton
{
    Q_OBJECT

public:
    enum SegmentType {
        LastSegment = 1,
        MiddleSegment = 2,
        FirstSegment = 4
    };

    explicit CrumblePathButton(const QString &title, QWidget *parent = 0);
    void setSegmentType(int type);
    void select(bool s);
    void setData(const QVariant &data);
    QVariant data() const;

protected:
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *e);
    void leaveEvent(QEvent *);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void changeEvent(QEvent * e);

private:
    void tintImages();

private:
    bool m_isHovering;
    bool m_isPressed;
    bool m_isSelected;
    bool m_isEnd;
    QColor m_baseColor;
    QImage m_segment;
    QImage m_segmentEnd;
    QImage m_segmentSelected;
    QImage m_segmentSelectedEnd;
    QImage m_segmentHover;
    QImage m_segmentHoverEnd;
    QImage m_triangleIcon;
    QPoint m_textPos;

    QVariant m_data;
};

CrumblePathButton::CrumblePathButton(const QString &title, QWidget *parent)
    : QPushButton(title, parent), m_isHovering(false), m_isPressed(false), m_isSelected(false), m_isEnd(true)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    setToolTip(title);
    setMinimumHeight(25);
    setMaximumHeight(25);
    setMouseTracking(true);
    m_textPos.setX(18);
    m_textPos.setY(height());
    m_baseColor = StyleHelper::baseColor();

    m_segment = QImage(QLatin1String(":/utils/images/crumblepath-segment.png"));
    m_segmentSelected = QImage(QLatin1String(":/utils/images/crumblepath-segment-selected.png"));
    m_segmentHover = QImage(QLatin1String(":/utils/images/crumblepath-segment-hover.png"));
    m_segmentEnd = QImage(QLatin1String(":/utils/images/crumblepath-segment-end.png"));
    m_segmentSelectedEnd = QImage(QLatin1String(":/utils/images/crumblepath-segment-selected-end.png"));
    m_segmentHoverEnd = QImage(QLatin1String(":/utils/images/crumblepath-segment-hover-end.png"));
    m_triangleIcon = QImage(QLatin1String(":/utils/images/triangle_vert.png"));

    tintImages();
}

void CrumblePathButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QRect geom(0, 0, geometry().width(), geometry().height());

    if (StyleHelper::baseColor() != m_baseColor) {
        m_baseColor = StyleHelper::baseColor();
        tintImages();
    }

    if (m_isEnd) {
        if (m_isPressed || m_isSelected)
            Utils::StyleHelper::drawCornerImage(m_segmentSelectedEnd, &p, geom, 2, 0, 2, 0);
        else if (m_isHovering)
            Utils::StyleHelper::drawCornerImage(m_segmentHoverEnd, &p, geom, 2, 0, 2, 0);
        else
            Utils::StyleHelper::drawCornerImage(m_segmentEnd, &p, geom, 2, 0, 2, 0);
    } else {
        if (m_isPressed || m_isSelected)
            Utils::StyleHelper::drawCornerImage(m_segmentSelected, &p, geom, 2, 0, 12, 0);
        else if (m_isHovering)
            Utils::StyleHelper::drawCornerImage(m_segmentHover, &p, geom, 2, 0, 12, 0);
        else
            Utils::StyleHelper::drawCornerImage(m_segment, &p, geom, 2, 0, 12, 0);
    }
    if (isEnabled())
        p.setPen(StyleHelper::panelTextColor());
    else
        p.setPen(StyleHelper::panelTextColor().darker());
    QFontMetrics fm(p.font());
    QString textToDraw = fm.elidedText(text(), Qt::ElideRight, geom.width() - m_textPos.x());

    p.drawText(QRectF(m_textPos.x(), 4, geom.width(), geom.height()), textToDraw);

    if (menu()) {
        p.drawImage(geom.width() - m_triangleIcon.width() - 6,
                    geom.center().y() - m_triangleIcon.height() / 2,
                    m_triangleIcon);
    }
}

void CrumblePathButton::tintImages()
{
    StyleHelper::tintImage(m_segmentEnd, m_baseColor);
    StyleHelper::tintImage(m_segmentSelectedEnd, m_baseColor);
    StyleHelper::tintImage(m_segmentHoverEnd, m_baseColor);
    StyleHelper::tintImage(m_segmentSelected, m_baseColor);
    StyleHelper::tintImage(m_segmentHover, m_baseColor);
    StyleHelper::tintImage(m_segment, m_baseColor);
}

void CrumblePathButton::leaveEvent(QEvent *e)
{
    m_isHovering = false;
    update();
    QPushButton::leaveEvent(e);
}

void CrumblePathButton::mouseMoveEvent(QMouseEvent *e)
{
    if (!isEnabled())
        return;
    m_isHovering = true;
    update();
    QPushButton::mouseMoveEvent(e);
}

void CrumblePathButton::mousePressEvent(QMouseEvent *e)
{
    if (!isEnabled())
        return;
    m_isPressed = true;
    update();
    QPushButton::mousePressEvent(e);
}

void CrumblePathButton::mouseReleaseEvent(QMouseEvent *e)
{
    m_isPressed = false;
    update();
    QPushButton::mouseReleaseEvent(e);
}

void CrumblePathButton::changeEvent(QEvent *e)
{
    if (e && e->type() == QEvent::EnabledChange)
        update();
}

void CrumblePathButton::select(bool s)
{
    m_isSelected = s;
    update();
}

void CrumblePathButton::setSegmentType(int type)
{
    bool useLeftPadding = !(type & FirstSegment);
    m_isEnd = (type & LastSegment);
    m_textPos.setX(useLeftPadding ? 18 : 4);
}

void CrumblePathButton::setData(const QVariant &data)
{
    m_data = data;
}

QVariant CrumblePathButton::data() const
{
    return m_data;
}

///////////////////////////////////////////////////////////////////////////////

struct CrumblePathPrivate
{
    explicit CrumblePathPrivate(CrumblePath *q);

    QList<CrumblePathButton*> m_buttons;
};

CrumblePathPrivate::CrumblePathPrivate(CrumblePath *q)
{
    Q_UNUSED(q)
}

//
// CrumblePath
//
CrumblePath::CrumblePath(QWidget *parent) :
    QWidget(parent), d(new CrumblePathPrivate(this))
{
    setMinimumHeight(25);
    setMaximumHeight(25);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

CrumblePath::~CrumblePath()
{
    qDeleteAll(d->m_buttons);
    d->m_buttons.clear();
    delete d;
}

void CrumblePath::selectIndex(int index)
{
    if (index > -1 && index < d->m_buttons.length())
        d->m_buttons[index]->select(true);
}

QVariant CrumblePath::dataForIndex(int index) const
{
    if (index > -1 && index < d->m_buttons.length())
        return d->m_buttons[index]->data();
    return QVariant();
}

QVariant CrumblePath::dataForLastIndex() const
{
    if (d->m_buttons.isEmpty())
        return QVariant();
    return d->m_buttons.last()->data();
}

int CrumblePath::length() const
{
    return d->m_buttons.length();
}

bool lessThan(const QAction *a1, const QAction *a2)
{
    return a1->text() < a2->text();
}

bool greaterThan(const QAction *a1, const QAction *a2)
{
    return a1->text() > a2->text();
}

void CrumblePath::sortChildren(Qt::SortOrder order)
{
    QPushButton *lastButton = d->m_buttons.last();

    QMenu *childList = lastButton->menu();
    QTC_ASSERT(childList, return);
    QList<QAction *> actions = childList->actions();

    if (order == Qt::AscendingOrder)
        qStableSort(actions.begin(), actions.end(), lessThan);
    else
        qStableSort(actions.begin(), actions.end(), greaterThan);

    childList->clear();
    childList->addActions(actions);
}

void CrumblePath::pushElement(const QString &title, const QVariant &data)
{
    CrumblePathButton *newButton = new CrumblePathButton(title, this);
    newButton->hide();
    connect(newButton, SIGNAL(clicked()), SLOT(emitElementClicked()));

    int segType = CrumblePathButton::MiddleSegment;
    if (!d->m_buttons.isEmpty()) {
        if (d->m_buttons.length() == 1)
            segType = segType | CrumblePathButton::FirstSegment;
        d->m_buttons.last()->setSegmentType(segType);
    } else {
        segType = CrumblePathButton::FirstSegment | CrumblePathButton::LastSegment;
        newButton->setSegmentType(segType);
    }
    newButton->setData(data);
    d->m_buttons.append(newButton);

    resizeButtons();
}

void CrumblePath::addChild(const QString &title, const QVariant &data)
{
    QTC_ASSERT(!d->m_buttons.isEmpty(), return);

    QPushButton *lastButton = d->m_buttons.last();

    QMenu *childList = lastButton->menu();
    if (childList == 0)
        childList = new QMenu(lastButton);

    QAction *childAction = new QAction(title, lastButton);
    childAction->setData(data);
    connect(childAction, SIGNAL(triggered()), this, SLOT(emitElementClicked()));
    childList->addAction(childAction);
    lastButton->setMenu(childList);
}

void CrumblePath::popElement()
{
    QWidget *last = d->m_buttons.last();
    d->m_buttons.removeLast();
    last->setParent(0);
    last->deleteLater();

    int segType = CrumblePathButton::MiddleSegment | CrumblePathButton::LastSegment;
    if (!d->m_buttons.isEmpty()) {
        if (d->m_buttons.length() == 1)
            segType = CrumblePathButton::FirstSegment | CrumblePathButton::LastSegment;
        d->m_buttons.last()->setSegmentType(segType);
    }
    resizeButtons();
}

void CrumblePath::clear()
{
    while (!d->m_buttons.isEmpty())
        popElement();
}

void CrumblePath::resizeEvent(QResizeEvent *)
{
    resizeButtons();
}

void CrumblePath::resizeButtons()
{
    int totalWidthLeft = width();

    if (!d->m_buttons.isEmpty()) {
        QPoint nextElementPosition(0, 0);

        d->m_buttons.first()->raise();
        // rearrange all items so that the first item is on top (added last).

        // compute relative sizes
        QList<int> sizes;
        int totalSize = 0;
        for (int i = 0; i < d->m_buttons.length() ; ++i) {
            CrumblePathButton *button = d->m_buttons.at(i);

            QFontMetrics fm(button->font());
            int originalSize = ArrowBorderSize + fm.width(button->text()) + ArrowBorderSize + 12;
            sizes << originalSize;
            totalSize += originalSize - ArrowBorderSize;
        }

        for (int i = 0; i < d->m_buttons.length() ; ++i) {
            CrumblePathButton *button = d->m_buttons.at(i);

            int candidateSize = (sizes.at(i) * totalWidthLeft) / totalSize;
            if (candidateSize < ArrowBorderSize)
                candidateSize = ArrowBorderSize;
            if (candidateSize > sizes.at(i) * 1.3)
                candidateSize = sizes.at(i) * 1.3;

            button->setMinimumWidth(candidateSize);
            button->setMaximumWidth(candidateSize);
            button->move(nextElementPosition);

            nextElementPosition.rx() += button->width() - ArrowBorderSize;

            button->show();
            if (i > 0) {
                // work-around for a compiler / optimization bug in i686-apple-darwin9-g
                // without volatile, the optimizer (-O2) seems to do the wrong thing (tm
                // the d->m_buttons array with an invalid argument.
                volatile int prevIndex = i - 1;
                button->stackUnder(d->m_buttons[prevIndex]);
            }
        }
    }
}

void CrumblePath::emitElementClicked()
{
    QObject *element = sender();
    if (QAction *action = qobject_cast<QAction*>(element))
        emit elementClicked(action->data());
    else if (CrumblePathButton *button = qobject_cast<CrumblePathButton*>(element))
        emit elementClicked(button->data());
}

} // namespace Utils

#include "crumblepath.moc"
