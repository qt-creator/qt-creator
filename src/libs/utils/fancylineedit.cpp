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

#include "fancylineedit.h"

#include <QtCore/QEvent>
#include <QtCore/QDebug>
#include <QtCore/QString>
#include <QtGui/QApplication>
#include <QtGui/QMenu>
#include <QtGui/QMouseEvent>
#include <QtGui/QLabel>
#include <QtGui/QAbstractButton>
#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtCore/QPropertyAnimation>


enum { margin = 6 };

#define ICONBUTTON_SIZE 18
#define FADE_TIME 160

namespace Utils {

// --------- FancyLineEditPrivate
class FancyLineEditPrivate : public QObject {
public:
    explicit FancyLineEditPrivate(FancyLineEdit *parent);

    virtual bool eventFilter(QObject *obj, QEvent *event);

    const QString m_leftLabelStyleSheet;
    const QString m_rightLabelStyleSheet;

    FancyLineEdit  *m_lineEdit;
    QPixmap m_pixmap;
    QMenu *m_menu;
    FancyLineEdit::Side m_side;
    bool m_useLayoutDirection;
    bool m_menuTabFocusTrigger;
    bool m_autoHideIcon;
    IconButton *m_iconbutton;
};


FancyLineEditPrivate::FancyLineEditPrivate(FancyLineEdit *parent) :
    QObject(parent),
    m_lineEdit(parent),
    m_menu(0),
    m_side(FancyLineEdit::Left),
    m_useLayoutDirection(false),
    m_menuTabFocusTrigger(false),
    m_autoHideIcon(false),
    m_iconbutton(new IconButton(parent))
{
}

bool FancyLineEditPrivate::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_iconbutton)
        return QObject::eventFilter(obj, event);
    switch (event->type()) {
    case QEvent::FocusIn:
        if (m_menuTabFocusTrigger && m_menu) {
            m_lineEdit->setFocus();
            m_menu->exec(m_iconbutton->mapToGlobal(m_iconbutton->rect().center()));
            return true;
        }
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}


// --------- FancyLineEdit
FancyLineEdit::FancyLineEdit(QWidget *parent) :
    QLineEdit(parent),
    m_d(new FancyLineEditPrivate(this))
{
    // KDE has custom icons for this. Notice that icon namings are counter intuitive
    // If these icons are not avaiable we use the freedesktop standard name before
    // falling back to a bundled resource
    QIcon icon = QIcon::fromTheme(layoutDirection() == Qt::LeftToRight ?
                     QLatin1String("edit-clear-locationbar-rtl") :
                     QLatin1String("edit-clear-locationbar-ltr"),
                     QIcon::fromTheme("edit-clear", QIcon(QLatin1String(":/core/images/editclear.png"))));

    m_d->m_iconbutton->installEventFilter(m_d);
    m_d->m_iconbutton->setIcon(icon);

    ensurePolished();
    setSide(Left);

    connect(this, SIGNAL(textChanged(QString)), this, SLOT(checkButton(QString)));
    connect(m_d->m_iconbutton, SIGNAL(clicked()), this, SLOT(iconClicked()));
}

void FancyLineEdit::checkButton(const QString &text)
{
    if (autoHideIcon()) {
        if (m_oldText.isEmpty() || text.isEmpty())
            m_d->m_iconbutton->animateShow(!text.isEmpty());
        m_oldText = text;
    }
}

FancyLineEdit::~FancyLineEdit()
{
}

void FancyLineEdit::setSide(Side side)
{
    m_d->m_side = side;

    Side iconpos = side;
    if (layoutDirection() == Qt::RightToLeft)
        iconpos = (side == Left ? Right : Left);

    // Make room for icon

    // Let the style determine minimum height for our widget
    QSize size(ICONBUTTON_SIZE + 6, ICONBUTTON_SIZE + 2);

    // Note KDE does not reserve space for the highlight color
    if (style()->inherits("OxygenStyle")) {
        size = size.expandedTo(QSize(24, 0));
    }

    QMargins margins;
    if (iconpos == Right)
        margins.setRight(size.width());
    else
        margins.setLeft(size.width());

    setTextMargins(margins);
}

void FancyLineEdit::iconClicked()
{
    if (m_d->m_menu) {
        m_d->m_menu->exec(QCursor::pos());
    } else {
        emit buttonClicked();
    }
}

FancyLineEdit::Side FancyLineEdit::side() const
{
    return  m_d->m_side;
}

void FancyLineEdit::resizeEvent(QResizeEvent *)
{
    QRect contentRect = rect();
    Side iconpos = m_d->m_side;
    if (layoutDirection() == Qt::RightToLeft)
        iconpos = (iconpos == Left ? Right : Left);

    if (iconpos == FancyLineEdit::Right) {
        const int iconoffset = textMargins().right() + 4;
        m_d->m_iconbutton->setGeometry(contentRect.adjusted(width() - iconoffset, 0, 0, 0));
    } else {
        const int iconoffset = textMargins().left() + 4;
        m_d->m_iconbutton->setGeometry(contentRect.adjusted(0, 0, -width() + iconoffset, 0));
    }
}

void FancyLineEdit::setPixmap(const QPixmap &pixmap)
{
    m_d->m_iconbutton->setIcon(pixmap);
    updateGeometry();
}

QPixmap FancyLineEdit::pixmap() const
{
    return m_d->m_pixmap;
}

void FancyLineEdit::setMenu(QMenu *menu)
{
     m_d->m_menu = menu;
     m_d->m_iconbutton->setIconOpacity(1.0);
 }

QMenu *FancyLineEdit::menu() const
{
    return  m_d->m_menu;
}

bool FancyLineEdit::hasMenuTabFocusTrigger() const
{
    return m_d->m_menuTabFocusTrigger;
}

void FancyLineEdit::setMenuTabFocusTrigger(bool v)
{
    if (m_d->m_menuTabFocusTrigger == v)
        return;

    m_d->m_menuTabFocusTrigger = v;
    m_d->m_iconbutton->setFocusPolicy(v ? Qt::TabFocus : Qt::NoFocus);
}

bool FancyLineEdit::autoHideIcon() const
{
    return m_d->m_autoHideIcon;
}

void FancyLineEdit::setAutoHideIcon(bool h)
{
    m_d->m_autoHideIcon = h;
    if (h)
        m_d->m_iconbutton->setIconOpacity(text().isEmpty() ?  0.0 : 1.0);
    else
        m_d->m_iconbutton->setIconOpacity(1.0);
}

void FancyLineEdit::setButtonToolTip(const QString &tip)
{
    m_d->m_iconbutton->setToolTip(tip);
}

void FancyLineEdit::setButtonFocusPolicy(Qt::FocusPolicy policy)
{
    m_d->m_iconbutton->setFocusPolicy(policy);
}

// IconButton - helper class to represent a clickable icon

IconButton::IconButton(QWidget *parent)
    : QAbstractButton(parent)
{
    setCursor(Qt::ArrowCursor);
    setFocusPolicy(Qt::NoFocus);
}

void IconButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    // Note isDown should really use the active state but in most styles
    // this has no proper feedback
    QIcon::Mode state = QIcon::Disabled;
    if (isEnabled())
        state = isDown() ? QIcon::Selected : QIcon::Normal;
    QPixmap iconpixmap = icon().pixmap(QSize(ICONBUTTON_SIZE, ICONBUTTON_SIZE),
                                       state, QIcon::Off);
    QRect pixmapRect = QRect(0, 0, iconpixmap.width(), iconpixmap.height());
    pixmapRect.moveCenter(rect().center());

    if (static_cast<FancyLineEdit*>(parentWidget())->autoHideIcon())
        painter.setOpacity(m_iconOpacity);

    painter.drawPixmap(pixmapRect, iconpixmap);
}

void IconButton::animateShow(bool visible)
{
    if (visible) {
        QPropertyAnimation *animation = new QPropertyAnimation(this, "iconOpacity");
        animation->setDuration(FADE_TIME);
        animation->setEndValue(1.0);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        QPropertyAnimation *animation = new QPropertyAnimation(this, "iconOpacity");
        animation->setDuration(FADE_TIME);
        animation->setEndValue(0.0);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

} // namespace Utils
