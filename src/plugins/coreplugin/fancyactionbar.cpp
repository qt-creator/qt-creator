/**************************************************************************
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

#include <QtGui/QHBoxLayout>
#include <QtGui/QPainter>
#include <QtGui/QPicture>
#include <QtGui/QVBoxLayout>
#include <QtSvg/QSvgRenderer>
#include <QtGui/QAction>

using namespace Core;
using namespace Internal;

static const char* const svgIdButtonBase =               "ButtonBase";
static const char* const svgIdButtonNormalBase =         "ButtonNormalBase";
static const char* const svgIdButtonNormalOverlay =      "ButtonNormalOverlay";
static const char* const svgIdButtonPressedBase =        "ButtonPressedBase";
static const char* const svgIdButtonPressedOverlay =     "ButtonPressedOverlay";
static const char* const svgIdButtonDisabledOverlay =    "ButtonDisabledOverlay";
static const char* const svgIdButtonHoverOverlay =       "ButtonHoverOverlay";

static const char* const elementsSvgIds[] = {
    svgIdButtonBase,
    svgIdButtonNormalBase,
    svgIdButtonNormalOverlay,
    svgIdButtonPressedBase,
    svgIdButtonPressedOverlay,
    svgIdButtonDisabledOverlay,
    svgIdButtonHoverOverlay
};

const QMap<QString, QPicture> &buttonElementsMap()
{
    static QMap<QString, QPicture> result;
    if (result.isEmpty()) {
        QSvgRenderer renderer(QLatin1String(":/fancyactionbar/images/fancytoolbutton.svg"));
        for (size_t i = 0; i < sizeof(elementsSvgIds)/sizeof(elementsSvgIds[0]); i++) {
            QString elementId(elementsSvgIds[i]);
            QPicture elementPicture;
            QPainter elementPainter(&elementPicture);
            renderer.render(&elementPainter, elementId);
            result.insert(elementId, elementPicture);
        }
    }
    return result;
}

FancyToolButton::FancyToolButton(QWidget *parent)
    : QToolButton(parent)
    , m_buttonElements(buttonElementsMap())
{
    setAttribute(Qt::WA_Hover, true);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
}

void FancyToolButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    QSize sh(sizeHint());
    double scale = (double)height() / sh.height();
    if (scale < 1) {
        p.save();
        p.scale(1, scale);
    }
    p.drawPicture(0, 0, m_buttonElements.value(svgIdButtonBase));
    p.drawPicture(0, 0, m_buttonElements.value(isDown() ? svgIdButtonPressedBase : svgIdButtonNormalBase));
#ifndef Q_WS_MAC // Mac UIs usually don't hover
    if (underMouse() && isEnabled())
        p.drawPicture(0, 0, m_buttonElements.value(svgIdButtonHoverOverlay));
#endif

    if (scale < 1)
        p.restore();

    if (!icon().isNull()) {
        icon().paint(&p, rect());
    } else {
        const int margin = 4;
        p.drawText(rect().adjusted(margin, margin, -margin, -margin), Qt::AlignCenter | Qt::TextWordWrap, text());

    }

    if (scale < 1) {
        p.scale(1, scale);
    }

    if (isEnabled()) {
        p.drawPicture(0, 0, m_buttonElements.value(isDown() ?
                                                   svgIdButtonPressedOverlay : svgIdButtonNormalOverlay));
    } else {
        p.drawPicture(0, 0, m_buttonElements.value(svgIdButtonDisabledOverlay));
    }
}

void FancyActionBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
}

QSize FancyToolButton::sizeHint() const
{
    return m_buttonElements.value(svgIdButtonBase).boundingRect().size();
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

    QHBoxLayout *centeringLayout = new QHBoxLayout;
    centeringLayout->addStretch();
    centeringLayout->addLayout(m_actionsLayout);
    centeringLayout->addStretch();
    setLayout(centeringLayout);
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
