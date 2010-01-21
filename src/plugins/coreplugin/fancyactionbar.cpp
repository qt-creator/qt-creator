/********************Q******************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <utils/stylehelper.h>

#include <coreplugin/icore.h>
#include <coreplugin/mainwindow.h>

#include <QtGui/QHBoxLayout>
#include <QtGui/QPainter>
#include <QtGui/QPicture>
#include <QtGui/QVBoxLayout>
#include <QtGui/QAction>
#include <QtGui/QStatusBar>
#include <QtGui/QStyle>
#include <QtGui/QStyleOption>

using namespace Core;
using namespace Internal;

FancyToolButton::FancyToolButton(QWidget *parent)
    : QToolButton(parent)
{
    setAttribute(Qt::WA_Hover, true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void FancyToolButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);

    QLayout *parentLayout = qobject_cast<FancyActionBar*>(parentWidget())->actionsLayout();
    int lineHeight = fontMetrics().height();
    bool isTitledAction = defaultAction()->property("titledAction").toBool();

#ifndef Q_WS_MAC // Mac UIs usually don't hover
    if (underMouse() && isEnabled() && !isDown()) {
        QColor whiteOverlay(Qt::white);
        whiteOverlay.setAlpha(20);
        p.fillRect(rect().adjusted(1,  1, -1, -1), whiteOverlay);
    }
#endif

    if (isDown()) {
        QColor whiteOverlay(Qt::black);
        whiteOverlay.setAlpha(20);
        p.fillRect(rect().adjusted(1,  1, -1, -1), whiteOverlay);
    }

        QPixmap borderPixmap;
        QMargins margins;
        if (parentLayout && parentLayout->count() > 0 &&
            parentLayout->itemAt(parentLayout->count()-1)->widget() == this) {
            margins = QMargins(3, 3, 2, 0);
            borderPixmap = QPixmap(
                    QLatin1String(":/fancyactionbar/images/fancytoolbutton_bottom_outline.png"));
        } else if (parentLayout && parentLayout->count() > 0 &&
                   parentLayout->itemAt(0)->widget() == this) {
            margins = QMargins(3, 3, 2, 3);
            borderPixmap = QPixmap(
                    QLatin1String(":/fancyactionbar/images/fancytoolbutton_top_outline.png"));
        } else {
            margins = QMargins(3, 3, 2, 0);
            borderPixmap = QPixmap(
                    QLatin1String(":/fancyactionbar/images/fancytoolbutton_normal_outline.png"));
        }

        QRect drawRect = rect();
        qDrawBorderPixmap(&p, drawRect, margins, borderPixmap);

        QPixmap pix = icon().pixmap(size() - QSize(15, 15), isEnabled() ? QIcon::Normal : QIcon::Disabled);
        QPoint center = rect().center();
        QSize halfPixSize = pix.size()/2;

    p.drawPixmap(center-QPoint(halfPixSize.width()-1, halfPixSize.height()-1), pix);

    if (popupMode() == QToolButton::DelayedPopup && !isTitledAction) {
        QPoint arrowOffset = center + QPoint(pix.rect().width()/2, pix.rect().height()/2);
        QStyleOption opt;
        if (isEnabled())
            opt.state &= QStyle::State_Enabled;
        else
            opt.state |= QStyle::State_Enabled;
        opt.rect = QRect(arrowOffset.x(), arrowOffset.y(), 6, 6);
        style()->drawPrimitive(QStyle::PE_IndicatorArrowDown,
                               &opt, &p, this);
    }

    if (isTitledAction) {
        QRect r(0, lineHeight/2, rect().width(), lineHeight);
        QColor penColor;
        if (isEnabled())
            penColor = Qt::white;
        else
            penColor = Qt::gray;
        p.setPen(penColor);
        const QString projectName = defaultAction()->property("heading").toString();
        QFont f = font();
        f.setPointSize(f.pointSize()-1);
        p.setFont(f);
        QFontMetrics fm(f);
        QString ellidedProjectName = fm.elidedText(projectName, Qt::ElideMiddle, r.width());
        if (isEnabled()) {
            const QRect shadowR = r.translated(0, 1);
            p.setPen(Qt::black);
            p.drawText(shadowR, Qt::AlignVCenter|Qt::AlignHCenter, ellidedProjectName);
            p.setPen(penColor);
        }
        p.drawText(r, Qt::AlignVCenter|Qt::AlignHCenter, ellidedProjectName);
        r = QRect(0, rect().bottom()-lineHeight*1.5, rect().width(), lineHeight);
        const QString buildConfiguration = defaultAction()->property("subtitle").toString();
        f.setBold(true);
        p.setFont(f);
        QString ellidedBuildConfiguration = fm.elidedText(buildConfiguration, Qt::ElideMiddle, r.width());
        if (isEnabled()) {
            const QRect shadowR = r.translated(0, 1);
            p.setPen(Qt::black);
            p.drawText(shadowR, Qt::AlignVCenter|Qt::AlignHCenter, ellidedBuildConfiguration);
            p.setPen(penColor);
        }
        p.drawText(r, Qt::AlignVCenter|Qt::AlignHCenter, ellidedBuildConfiguration);
    }

}

void FancyActionBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
}

QSize FancyToolButton::sizeHint() const
{
    QSize buttonSize = iconSize().expandedTo(QSize(64, 40));
    if (defaultAction()->property("titledAction").toBool()) {
        int lineHeight = fontMetrics().height();
        buttonSize += QSize(0, lineHeight*4);
    }
    return buttonSize;
}

QSize FancyToolButton::minimumSizeHint() const
{
    return QSize(8, 8);
}

void FancyToolButton::actionChanged()
{
    // the default action changed in some way, e.g. it might got hidden
    // since we inherit a tool button we won't get invisible, so do this here
    if (QAction* action = defaultAction())
        setVisible(action->isVisible());
}

FancyActionBar::FancyActionBar(QWidget *parent)
    : QWidget(parent)
{
    m_actionsLayout = new QVBoxLayout;

    QVBoxLayout *spacerLayout = new QVBoxLayout;
    spacerLayout->addLayout(m_actionsLayout);
    int sbh = ICore::instance()->statusBar()->height();
    spacerLayout->addSpacing(sbh);
    spacerLayout->setMargin(0);
    spacerLayout->setSpacing(0);

    QHBoxLayout *orientRightLayout = new QHBoxLayout;
    orientRightLayout->addStretch();
    orientRightLayout->setMargin(0);
    orientRightLayout->setSpacing(0);
    orientRightLayout->setContentsMargins(0, 0, 1, 0);
    orientRightLayout->addLayout(spacerLayout);
    setLayout(orientRightLayout);
}

void FancyActionBar::addProjectSelector(QAction *action)
{
    FancyToolButton* toolButton = new FancyToolButton(this);
    toolButton->setDefaultAction(action);
    connect(action, SIGNAL(changed()), toolButton, SLOT(actionChanged()));
    m_actionsLayout->insertWidget(0, toolButton);

}
void FancyActionBar::insertAction(int index, QAction *action, QMenu *menu)
{
    FancyToolButton *toolButton = new FancyToolButton(this);
    toolButton->setDefaultAction(action);
    connect(action, SIGNAL(changed()), toolButton, SLOT(actionChanged()));

    if (menu) {
        toolButton->setMenu(menu);
        toolButton->setPopupMode(QToolButton::DelayedPopup);

        // execute action also if a context menu item is select
        connect(toolButton, SIGNAL(triggered(QAction*)),
                this, SLOT(toolButtonContextMenuActionTriggered(QAction*)),
                Qt::QueuedConnection);
    }
    m_actionsLayout->insertWidget(index, toolButton);
}

/*
  This slot is invoked when a context menu action of a tool button is triggered.
  In this case we also want to trigger the default action of the button.

  This allows the user e.g. to select and run a specific run configuration with one click.
  */
void FancyActionBar::toolButtonContextMenuActionTriggered(QAction* action)
{
    if (QToolButton *button = qobject_cast<QToolButton*>(sender())) {
        if (action != button->defaultAction())
            button->defaultAction()->trigger();
    }
}

QLayout *FancyActionBar::actionsLayout() const
{
    return m_actionsLayout;
}
