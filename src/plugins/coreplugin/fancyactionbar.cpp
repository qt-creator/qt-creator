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

#include "fancyactionbar.h"
#include "coreconstants.h"

#include <utils/stylehelper.h>

#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/mainwindow.h>

#include <QtGui/QHBoxLayout>
#include <QtGui/QPainter>
#include <QtGui/QPicture>
#include <QtGui/QVBoxLayout>
#include <QtGui/QAction>
#include <QtGui/QStatusBar>
#include <QtGui/QStyle>
#include <QtGui/QStyleOption>
#include <QtGui/QMouseEvent>
#include <QtGui/QApplication>
#include <QtCore/QEvent>
#include <QtCore/QAnimationGroup>
#include <QtCore/QPropertyAnimation>
#include <QtCore/QDebug>

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
    switch(e->type()) {
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
    default:
        return QToolButton::event(e);
    }
    return false;
}

void FancyToolButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);

    // draw borders
    bool isTitledAction = defaultAction()->property("titledAction").toBool();

#ifndef Q_WS_MAC // Mac UIs usually don't hover
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

        const QString buildConfiguration = defaultAction()->property("subtitle").toString();
        if (!buildConfiguration.isNull())
            centerRect.adjust(0, 0, 0, -lineHeight - 4);

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

        QString ellidedProjectName = fm.elidedText(projectName, Qt::ElideMiddle, r.width() - 6);
        if (isEnabled()) {
            const QRectF shadowR = r.translated(0, 1);
            painter.setPen(QColor(30, 30, 30, 80));
            painter.drawText(shadowR, textFlags, ellidedProjectName);
            painter.setPen(penColor);
        }
        painter.drawText(r, textFlags, ellidedProjectName);
        textOffset = iconRect.center() + QPoint(iconRect.width()/2, iconRect.height()/2);
        r = QRectF(0, textOffset.y()+5, rect().width(), lineHeight);
        painter.setFont(boldFont);
        QString ellidedBuildConfiguration = boldFm.elidedText(buildConfiguration, Qt::ElideMiddle, r.width());
        if (isEnabled()) {
            const QRectF shadowR = r.translated(0, 1);
            painter.setPen(QColor(30, 30, 30, 80));
            painter.drawText(shadowR, textFlags, ellidedBuildConfiguration);
            painter.setPen(penColor);
        }
        if (!icon().isNull()) {
            painter.drawText(r, textFlags, ellidedBuildConfiguration);
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

        const QString buildConfiguration = defaultAction()->property("subtitle").toString();
        if (!buildConfiguration.isEmpty())
            buttonSize += QSizeF(0, lineHeight + 2);
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
    setObjectName(QString::fromUtf8("actionbar"));
    m_actionsLayout = new QVBoxLayout;
    QVBoxLayout *spacerLayout = new QVBoxLayout;
    spacerLayout->addLayout(m_actionsLayout);
    int sbh = ICore::instance()->statusBar()->height();
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
