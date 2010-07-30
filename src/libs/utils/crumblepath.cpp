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

#include <QHBoxLayout>
#include <QPushButton>
#include <QStyle>
#include <QResizeEvent>

namespace Utils {

static const int ArrowBorderSize = 12;

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
private:
    static QString middleSegmentSheet(bool useLeftPadding);
    static QString lastSegmentSheet(bool useLeftPadding);
};

CrumblePathButton::CrumblePathButton(const QString &title, QWidget *parent)
    : QPushButton(title, parent)
{
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    setToolTip(title);
    setMinimumHeight(24);
    setMaximumHeight(24);
    setStyleSheet(lastSegmentSheet(true));
}

QString CrumblePathButton::middleSegmentSheet(bool useLeftPadding)
{
    QString sheet = QString(
                "QPushButton {"
                "text-align: left;"
                "color: #eeeeee;"
                "border-image: url(:/qml/images/segment.png)  0 12 0 2;"
                "border-width: 0 12px 0 2px;"
                "padding-left:%1px;"
                "}"
                "QPushButton:hover {"
                "    border-image: url(:/qml/images/segment-hover.png)  0 12 0 2;"
                "}"
                "QPushButton:pressed {"
                "    border-image: url(:/qml/images/segment-selected.png)  0 12 0 2;"
                "}"
                ).arg(useLeftPadding ? "18" : "4");
    return sheet;
}

QString CrumblePathButton::lastSegmentSheet(bool useLeftPadding)
{
    QString sheet = QString(
                "QPushButton {"
                "text-align: left;"
                "color: #eeeeee;"
                "border-image: url(:/qml/images/segment-end.png)  0 2 0 2;"
                "border-width: 0 2px 0 2px;"
                "padding-left:%1px;"
                "}"
                "QPushButton:hover {"
                "    border-image: url(:/qml/images/segment-hover-end.png)  0 2 0 2;"
                "}"
                "QPushButton:pressed {"
                "    border-image: url(:/qml/images/segment-selected-end.png)  0 2 0 2;"
                "}"
                ).arg(useLeftPadding ? "18" : "4");
    return sheet;
}

void CrumblePathButton::setSegmentType(int type)
{
    bool useLeftPadding = !(type & FirstSegment);
    if (type & LastSegment) {
        setStyleSheet(lastSegmentSheet(useLeftPadding));
    } else if (type & MiddleSegment) {
        setStyleSheet(middleSegmentSheet(useLeftPadding));
    }
}

//
// CrumblePath
//
CrumblePath::CrumblePath(QWidget *parent) :
    QWidget(parent), m_background(new QWidget(this))
{
    setMinimumHeight(25);
    setMaximumHeight(25);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    m_background->setStyleSheet("QWidget { background-color:#2d2d2d;}");
    m_background->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
}

CrumblePath::~CrumblePath()
{
    qDeleteAll(m_buttons);
    m_buttons.clear();
}

void CrumblePath::pushElement(const QString &title)
{
    CrumblePathButton *newButton = new CrumblePathButton(title, this);
    newButton->hide();
    connect(newButton, SIGNAL(clicked()), SLOT(mapClickToIndex()));
    connect(newButton, SIGNAL(customContextMenuRequested(QPoint)), SLOT(mapContextMenuRequestToIndex()));

    int segType = CrumblePathButton::MiddleSegment;
    if (!m_buttons.isEmpty()) {
        if (m_buttons.length() == 1)
            segType = segType | CrumblePathButton::FirstSegment;
        m_buttons.last()->setSegmentType(segType);
    } else {
        segType = CrumblePathButton::FirstSegment | CrumblePathButton::LastSegment;
        newButton->setSegmentType(segType);
    }
    m_buttons.append(newButton);

    resizeButtons();
}

void CrumblePath::popElement()
{
    QWidget *last = m_buttons.last();
    m_buttons.removeLast();
    last->setParent(0);
    last->deleteLater();

    int segType = CrumblePathButton::MiddleSegment | CrumblePathButton::LastSegment;
    if (!m_buttons.isEmpty()) {
        if (m_buttons.length() == 1)
            segType = CrumblePathButton::FirstSegment | CrumblePathButton::LastSegment;
        m_buttons.last()->setSegmentType(segType);
    }
    resizeButtons();
}

void CrumblePath::clear()
{
    while (!m_buttons.isEmpty()) {
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

    if (m_buttons.length() >= 1) {
        QPoint nextElementPosition(0,0);

        m_buttons[0]->raise();
        // rearrange all items so that the first item is on top (added last).
        for(int i = 0; i < m_buttons.length() ; ++i) {
            CrumblePathButton *button = m_buttons[i];

            QFontMetrics fm(button->font());
            buttonMinWidth = ArrowBorderSize + fm.width(button->text()) + ArrowBorderSize * 2 ;
            buttonMaxWidth = (totalWidthLeft + ArrowBorderSize * (m_buttons.length() - i)) / (m_buttons.length() - i);

            if (buttonMinWidth > buttonMaxWidth && i < m_buttons.length() - 1) {
                buttonMinWidth = buttonMaxWidth;
            } else if (i > 3 && (i == m_buttons.length() - 1)) {
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
                button->stackUnder(m_buttons[i - 1]);
        }

    }

    m_background->setGeometry(0,0, width(), height());
    m_background->update();
}

void CrumblePath::mapClickToIndex()
{
    QObject *element = sender();
    for (int i = 0; i < m_buttons.length(); ++i) {
        if (m_buttons[i] == element) {
            emit elementClicked(i);
            return;
        }
    }
}

void CrumblePath::mapContextMenuRequestToIndex()
{
    QObject *element = sender();
    for (int i = 0; i < m_buttons.length(); ++i) {
        if (m_buttons[i] == element) {
            emit elementContextMenuRequested(i);
            return;
        }
    }
}

} // namespace QmlViewer
