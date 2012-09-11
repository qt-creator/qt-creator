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

#include "kitmanagerconfigwidget.h"

#include "kit.h"

#include <utils/detailswidget.h>

#include <QHBoxLayout>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStyle>

namespace ProjectExplorer {

void KitConfigWidget::addToLayout(QGridLayout *layout, int row)
{
    addLabel(layout, row);
    layout->addWidget(this, row, WidgetColumn);
    addButtonWidget(layout, row);
}

void KitConfigWidget::addLabel(QGridLayout *layout, int row)
{
    static const Qt::Alignment alignment
        = static_cast<Qt::Alignment>(style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
    QLabel *label = new QLabel(displayName());
    label->setToolTip(toolTip());
    layout->addWidget(label, row, LabelColumn, alignment);
}

void KitConfigWidget::addButtonWidget(QGridLayout *layout, int row)
{
    if (QWidget *button = buttonWidget()) {
        if (button->toolTip().isEmpty())
            button->setToolTip(toolTip());
        layout->addWidget(button, row, ButtonColumn);
    }
}

namespace Internal {

KitManagerConfigWidget::KitManagerConfigWidget(Kit *k, QWidget *parent) :
    KitConfigWidget(parent),
    m_layout(new QGridLayout),
    m_iconButton(new QToolButton),
    m_kit(k)
{
    m_layout->setMargin(0);
    m_layout->setSpacing(6);
    m_layout->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *top = new QVBoxLayout(this);
    top->setMargin(0);

    QScrollArea *scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setFocusPolicy(Qt::NoFocus);
    top->addWidget(scroll);

    Utils::DetailsWidget *details = new Utils::DetailsWidget;
    details->setState(Utils::DetailsWidget::NoSummary);
    scroll->setWidget(details);

    QWidget *widget = new QWidget;
    details->setWidget(widget);

    QVBoxLayout *iconLayout = new QVBoxLayout;
    iconLayout->addWidget(m_iconButton);
    iconLayout->addStretch();

    QGridLayout *masterLayout = new QGridLayout(widget);
    masterLayout->setMargin(0);
    masterLayout->setContentsMargins(6, 0, 6, 0);
    masterLayout->addLayout(iconLayout, 0, 0);
    masterLayout->addLayout(m_layout, 0, 1);
    masterLayout->setRowStretch(1, 1);

    discard();

    connect(m_iconButton, SIGNAL(clicked()), this, SLOT(setIcon()));
}

QString KitManagerConfigWidget::displayName() const
{
    return tr("Kits");
}

void KitManagerConfigWidget::apply()
{
    foreach (KitConfigWidget *w, m_widgets)
        w->apply();
    m_kit->setIconPath(m_iconPath);
}

void KitManagerConfigWidget::discard()
{
    foreach (KitConfigWidget *w, m_widgets)
        w->discard();
    m_iconButton->setIcon(m_kit->icon());
    m_iconPath = m_kit->iconPath();
}

bool KitManagerConfigWidget::isDirty() const
{
    foreach (KitConfigWidget *w, m_widgets)
        if (w->isDirty())
            return true;
    return m_kit->iconPath() != m_iconPath;
}

void KitManagerConfigWidget::addConfigWidget(ProjectExplorer::KitConfigWidget *widget)
{
    Q_ASSERT(widget);
    Q_ASSERT(!m_widgets.contains(widget));

    connect(widget, SIGNAL(dirty()), this, SIGNAL(dirty()));
    widget->addToLayout(m_layout, m_layout->rowCount());
    m_widgets.append(widget);
}

void KitManagerConfigWidget::makeReadOnly()
{
    foreach (KitConfigWidget *w, m_widgets)
        w->makeReadOnly();
    m_iconButton->setEnabled(false);
}

void KitManagerConfigWidget::setIcon()
{
    const QString path = QFileDialog::getOpenFileName(0, tr("Select Icon"), m_iconPath, tr("Images (*.png *.xpm *.jpg)"));
    if (path.isEmpty())
        return;

    const QIcon icon = QIcon(path);
    if (icon.isNull())
        return;

    m_iconButton->setIcon(icon);
    m_iconPath = path;
    emit dirty();
}

} // namespace Internal
} // namespace ProjectExplorer
