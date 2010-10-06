/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "crumblepath.h"

#include <QtCore/QList>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QStyle>
#include <QtGui/QResizeEvent>
#include <QtGui/QPainter>
#include <QtGui/QImage>

static const int ArrowBorderSize = 12;

// Draws a CSS-like border image where the defined borders are not stretched
static void drawCornerImage(const QImage &img, QPainter *painter, QRect rect,
                                  int left, int top, int right, int bottom)
{
    QSize size = img.size();
    if (top > 0) { //top
        painter->drawImage(QRect(rect.left() + left, rect.top(), rect.width() -right - left, top), img,
                           QRect(left, 0, size.width() -right - left, top));
        if (left > 0) //top-left
            painter->drawImage(QRect(rect.left(), rect.top(), left, top), img,
                               QRect(0, 0, left, top));
        if (right > 0) //top-right
            painter->drawImage(QRect(rect.left() + rect.width() - right, rect.top(), right, top), img,
                               QRect(size.width() - right, 0, right, top));
    }
    //left
    if (left > 0)
        painter->drawImage(QRect(rect.left(), rect.top()+top, left, rect.height() - top - bottom), img,
                           QRect(0, top, left, size.height() - bottom - top));
    //center
    painter->drawImage(QRect(rect.left() + left, rect.top()+top, rect.width() -right - left,
                             rect.height() - bottom - top), img,
                       QRect(left, top, size.width() -right -left,
                             size.height() - bottom - top));
    if (right > 0) //right
        painter->drawImage(QRect(rect.left() +rect.width() - right, rect.top()+top, right, rect.height() - top - bottom), img,
                           QRect(size.width() - right, top, right, size.height() - bottom - top));
    if (bottom > 0) { //bottom
        painter->drawImage(QRect(rect.left() +left, rect.top() + rect.height() - bottom,
                                 rect.width() - right - left, bottom), img,
                           QRect(left, size.height() - bottom,
                                 size.width() - right - left, bottom));
    if (left > 0) //bottom-left
        painter->drawImage(QRect(rect.left(), rect.top() + rect.height() - bottom, left, bottom), img,
                           QRect(0, size.height() - bottom, left, bottom));
    if (right > 0) //bottom-right
        painter->drawImage(QRect(rect.left() + rect.width() - right, rect.top() + rect.height() - bottom, right, bottom), img,
                           QRect(size.width() - right, size.height() - bottom, right, bottom));
    }
}

// Tints an image with tintColor, while preserving alpha and lightness
static void tintImage(QImage &img, const QColor &tintColor)
{
    QPainter p(&img);
    p.setCompositionMode(QPainter::CompositionMode_Screen);

    for (int x = 0; x < img.width(); ++x) {
        for (int y = 0; y < img.height(); ++y) {
            QRgb rgbColor = img.pixel(x, y);
            int alpha = qAlpha(rgbColor);
            QColor c = QColor(rgbColor);

            if (alpha > 0) {
                c.toHsl();
                qreal l = c.lightnessF();
                QColor newColor = QColor::fromHslF(tintColor.hslHueF(), tintColor.hslSaturationF(), l);
                newColor.setAlpha(alpha);
                img.setPixel(x, y, newColor.rgba());
            }
        }
    }
}

class CrumblePathButton : public QPushButton
{
public:
    enum SegmentType {
        LastSegment = 1,
        MiddleSegment = 2,
        FirstSegment = 4
    };

    explicit CrumblePathButton(const QString &title, QWidget *parent = 0);
    void setSegmentType(int type);
protected:
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *e);
    void leaveEvent(QEvent *);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

private:
    void tintImages();

private:
    bool m_isHovering;
    bool m_isPressed;
    bool m_isEnd;
    QColor m_baseColor;
    QImage m_segment;
    QImage m_segmentEnd;
    QImage m_segmentSelected;
    QImage m_segmentSelectedEnd;
    QImage m_segmentHover;
    QImage m_segmentHoverEnd;
    QPoint m_textPos;
};

CrumblePathButton::CrumblePathButton(const QString &title, QWidget *parent)
    : QPushButton(title, parent), m_isHovering(false), m_isPressed(false), m_isEnd(true)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    setToolTip(title);
    setMinimumHeight(24);
    setMaximumHeight(24);
    setMouseTracking(true);
    m_textPos.setX(18);
    m_textPos.setY(height());
    m_baseColor = QColor(0x666666);

    m_segment = QImage(":/crumblepath/images/crumblepath-segment.png");
    m_segmentSelected = QImage(":/crumblepath/images/crumblepath-segment-selected.png");
    m_segmentHover = QImage(":/crumblepath/images/crumblepath-segment-hover.png");
    m_segmentEnd = QImage(":/crumblepath/images/crumblepath-segment-end.png");
    m_segmentSelectedEnd = QImage(":/crumblepath/images/crumblepath-segment-selected-end.png");
    m_segmentHoverEnd = QImage(":/crumblepath/images/crumblepath-segment-hover-end.png");

    tintImages();
}

void CrumblePathButton::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QRect geom(0, 0, geometry().width(), geometry().height());

    if (m_isEnd) {
        if (m_isPressed) {
            drawCornerImage(m_segmentSelectedEnd, &p, geom, 2, 0, 2, 0);
        } else if (m_isHovering) {
            drawCornerImage(m_segmentHoverEnd, &p, geom, 2, 0, 2, 0);
        } else {
            drawCornerImage(m_segmentEnd, &p, geom, 2, 0, 2, 0);
        }
    } else {
        if (m_isPressed) {
            drawCornerImage(m_segmentSelected, &p, geom, 2, 0, 12, 0);
        } else if (m_isHovering) {
            drawCornerImage(m_segmentHover, &p, geom, 2, 0, 12, 0);
        } else {
            drawCornerImage(m_segment, &p, geom, 2, 0, 12, 0);
        }
    }
    p.setPen(QColor(Qt::white));
    QFontMetrics fm(p.font());
    QString textToDraw = fm.elidedText(text(), Qt::ElideRight, geom.width() - m_textPos.x());

    p.drawText(QRectF(m_textPos.x(), 4, geom.width(), geom.height()), textToDraw);
}

void CrumblePathButton::tintImages()
{
    tintImage(m_segmentEnd, m_baseColor);
    tintImage(m_segmentSelectedEnd, m_baseColor);
    tintImage(m_segmentHoverEnd, m_baseColor);
    tintImage(m_segmentSelected, m_baseColor);
    tintImage(m_segmentHover, m_baseColor);
    tintImage(m_segment, m_baseColor);
}

void CrumblePathButton::leaveEvent(QEvent *e)
{
    QPushButton::leaveEvent(e);
    m_isHovering = false;
    update();
}

void CrumblePathButton::mouseMoveEvent(QMouseEvent *e)
{
    QPushButton::mouseMoveEvent(e);
    m_isHovering = true;
    update();
}

void CrumblePathButton::mousePressEvent(QMouseEvent *e)
{
    QPushButton::mousePressEvent(e);
    m_isPressed = true;
    update();
}

void CrumblePathButton::mouseReleaseEvent(QMouseEvent *e)
{
    QPushButton::mouseReleaseEvent(e);
    m_isPressed = false;
    update();
}

void CrumblePathButton::setSegmentType(int type)
{
    bool useLeftPadding = !(type & FirstSegment);
    m_isEnd = (type & LastSegment);
    m_textPos.setX(useLeftPadding ? 18 : 4);
}

struct CrumblePathPrivate {
    explicit CrumblePathPrivate(CrumblePath *q);

    QColor m_baseColor;
    QList<CrumblePathButton*> m_buttons;
    QWidget *m_background;
};

CrumblePathPrivate::CrumblePathPrivate(CrumblePath *q) :
    m_baseColor(0x666666),
    m_background(new QWidget(q))
{
}

//
// CrumblePath
//
CrumblePath::CrumblePath(QWidget *parent) :
    QWidget(parent), d(new CrumblePathPrivate(this))
{
    setMinimumHeight(25);
    setMaximumHeight(25);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    setBackgroundStyle();
    d->m_background->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
}

CrumblePath::~CrumblePath()
{
    qDeleteAll(d->m_buttons);
    d->m_buttons.clear();
}

void CrumblePath::setBackgroundStyle()
{
    d->m_background->setStyleSheet("QWidget { background-color:" + d->m_baseColor.name() + ";}");
}

void CrumblePath::pushElement(const QString &title)
{
    CrumblePathButton *newButton = new CrumblePathButton(title, this);
    newButton->hide();
    connect(newButton, SIGNAL(clicked()), SLOT(mapClickToIndex()));
    connect(newButton, SIGNAL(customContextMenuRequested(QPoint)), SLOT(mapContextMenuRequestToIndex()));

    int segType = CrumblePathButton::MiddleSegment;
    if (!d->m_buttons.isEmpty()) {
        if (d->m_buttons.length() == 1)
            segType = segType | CrumblePathButton::FirstSegment;
        d->m_buttons.last()->setSegmentType(segType);
    } else {
        segType = CrumblePathButton::FirstSegment | CrumblePathButton::LastSegment;
        newButton->setSegmentType(segType);
    }
    d->m_buttons.append(newButton);

    resizeButtons();
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
    while (!d->m_buttons.isEmpty()) {
        popElement();
    }
}

void CrumblePath::resizeEvent(QResizeEvent *)
{
    resizeButtons();
}

void CrumblePath::resizeButtons()
{
    int buttonMinWidth = 0;
    int buttonMaxWidth = 0;
    int totalWidthLeft = width();

    if (d->m_buttons.length() >= 1) {
        QPoint nextElementPosition(0,0);

        d->m_buttons[0]->raise();
        // rearrange all items so that the first item is on top (added last).
        for(int i = 0; i < d->m_buttons.length() ; ++i) {
            CrumblePathButton *button = d->m_buttons[i];

            QFontMetrics fm(button->font());
            buttonMinWidth = ArrowBorderSize + fm.width(button->text()) + ArrowBorderSize * 2 ;
            buttonMaxWidth = (totalWidthLeft + ArrowBorderSize * (d->m_buttons.length() - i)) / (d->m_buttons.length() - i);

            if (buttonMinWidth > buttonMaxWidth && i < d->m_buttons.length() - 1) {
                buttonMinWidth = buttonMaxWidth;
            } else if (i > 3 && (i == d->m_buttons.length() - 1)) {
                buttonMinWidth = width() - nextElementPosition.x();
                buttonMaxWidth = buttonMinWidth;
            }

            button->setMinimumWidth(buttonMinWidth);
            button->setMaximumWidth(buttonMaxWidth);
            button->move(nextElementPosition);

            nextElementPosition.rx() += button->width() - ArrowBorderSize;
            totalWidthLeft -= button->width();

            button->show();
            if (i > 0)
                button->stackUnder(d->m_buttons[i - 1]);
        }

    }

    d->m_background->setGeometry(0,0, width(), height());
    d->m_background->update();
}

void CrumblePath::mapClickToIndex()
{
    QObject *element = sender();
    for (int i = 0; i < d->m_buttons.length(); ++i) {
        if (d->m_buttons[i] == element) {
            emit elementClicked(i);
            return;
        }
    }
}

void CrumblePath::mapContextMenuRequestToIndex()
{
    QObject *element = sender();
    for (int i = 0; i < d->m_buttons.length(); ++i) {
        if (d->m_buttons[i] == element) {
            emit elementContextMenuRequested(i);
            return;
        }
    }
}

