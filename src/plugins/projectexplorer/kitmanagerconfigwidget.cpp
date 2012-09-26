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
#include <QLineEdit>
#include <QToolButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStyle>

namespace ProjectExplorer {

namespace Internal {

KitManagerConfigWidget::KitManagerConfigWidget(Kit *k, QWidget *parent) :
    KitConfigWidget(parent),
    m_layout(new QGridLayout),
    m_iconButton(new QToolButton),
    m_nameEdit(new QLineEdit),
    m_kit(k)
{
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

    m_layout->setMargin(0);
    m_layout->setSpacing(6);
    m_layout->setContentsMargins(6, 0, 6, 0);
    m_layout->setRowStretch(1, 1);
    widget->setLayout(m_layout);

    details->setWidget(widget);

    addToLayout(tr("Name:"), tr("Kit name and icon."), m_nameEdit, m_iconButton);
    discard();

    connect(m_iconButton, SIGNAL(clicked()), this, SLOT(setIcon()));
    connect(m_nameEdit, SIGNAL(textChanged(QString)), this, SIGNAL(dirty()));
}

QString KitManagerConfigWidget::displayName() const
{
    return m_nameEdit->text();
}

void KitManagerConfigWidget::apply()
{
    foreach (KitConfigWidget *w, m_widgets)
        w->apply();
    m_kit->setIconPath(m_iconPath);
    m_kit->setDisplayName(m_nameEdit->text());
}

void KitManagerConfigWidget::discard()
{
    foreach (KitConfigWidget *w, m_widgets)
        w->discard();
    m_iconButton->setIcon(m_kit->icon());
    m_iconPath = m_kit->iconPath();
    m_nameEdit->setText(m_kit->displayName());
}

bool KitManagerConfigWidget::isDirty() const
{
    foreach (KitConfigWidget *w, m_widgets)
        if (w->isDirty())
            return true;
    return (m_kit->iconPath() != m_iconPath) || (m_kit->displayName() != m_nameEdit->text());
}

void KitManagerConfigWidget::addConfigWidget(ProjectExplorer::KitConfigWidget *widget)
{
    Q_ASSERT(widget);
    Q_ASSERT(!m_widgets.contains(widget));

    connect(widget, SIGNAL(dirty()), this, SIGNAL(dirty()));

    addToLayout(widget->displayName(), widget->toolTip(), widget, widget->buttonWidget());
    m_widgets.append(widget);
}

void KitManagerConfigWidget::makeReadOnly()
{
    foreach (KitConfigWidget *w, m_widgets)
        w->makeReadOnly();
    m_iconButton->setEnabled(false);
    m_nameEdit->setEnabled(false);
}

void KitManagerConfigWidget::setIcon()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Select Icon"), m_iconPath, tr("Images (*.png *.xpm *.jpg)"));
    if (path.isEmpty())
        return;

    const QIcon icon = QIcon(path);
    if (icon.isNull())
        return;

    m_iconButton->setIcon(icon);
    m_iconPath = path;
    emit dirty();
}

void KitManagerConfigWidget::addToLayout(const QString &name, const QString &toolTip,
                                         QWidget *widget, QWidget *button)
{
    int row = m_layout->rowCount();
    addLabel(name, toolTip, row);
    m_layout->addWidget(widget, row, WidgetColumn);
    addButtonWidget(button, toolTip, row);
}

void KitManagerConfigWidget::addLabel(const QString &name, const QString &toolTip, int row)
{
    static const Qt::Alignment alignment
        = static_cast<Qt::Alignment>(style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
    QLabel *label = new QLabel(name);
    label->setToolTip(toolTip);
    m_layout->addWidget(label, row, LabelColumn, alignment);
}

void KitManagerConfigWidget::addButtonWidget(QWidget *button, const QString &toolTip, int row)
{
    if (!button)
        return;
    if (button->toolTip().isEmpty())
        button->setToolTip(toolTip);
    m_layout->addWidget(button, row, ButtonColumn);
}

} // namespace Internal
} // namespace ProjectExplorer
