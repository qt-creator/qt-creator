/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "kitmanagerconfigwidget.h"

#include "kit.h"
#include "kitmanager.h"

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

static const char WORKING_COPY_KIT_ID[] = "modified kit";

namespace ProjectExplorer {

namespace Internal {

KitManagerConfigWidget::KitManagerConfigWidget(Kit *k, QWidget *parent) :
    QWidget(parent),
    m_layout(new QGridLayout),
    m_iconButton(new QToolButton),
    m_nameEdit(new QLineEdit),
    m_kit(k),
    m_modifiedKit(new Kit(Core::Id(WORKING_COPY_KIT_ID))),
    m_fixingKit(false)
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
    connect(m_nameEdit, SIGNAL(textChanged(QString)), this, SLOT(setDisplayName()));

    KitManager *km = KitManager::instance();
    connect(km, SIGNAL(unmanagedKitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(workingCopyWasUpdated(ProjectExplorer::Kit*)));
    connect(km, SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(kitWasUpdated(ProjectExplorer::Kit*)));
}

KitManagerConfigWidget::~KitManagerConfigWidget()
{
    delete m_modifiedKit;
    // Make sure our workingCopy did not get registered somehow:
    foreach (const Kit *k, KitManager::instance()->kits())
        Q_ASSERT(k->id() != Core::Id(WORKING_COPY_KIT_ID));
}

QString KitManagerConfigWidget::displayName() const
{
    return m_nameEdit->text();
}

void KitManagerConfigWidget::apply()
{
    KitManager *km = KitManager::instance();
    bool mustRegister = false;
    if (!m_kit) {
        mustRegister = true;
        m_kit = new Kit;
    }
    m_kit->copyFrom(m_modifiedKit);
    if (mustRegister)
        km->registerKit(m_kit);

    if (m_isDefaultKit)
        km->setDefaultKit(m_kit);
    emit dirty();
}

void KitManagerConfigWidget::discard()
{
    if (m_kit) {
        m_modifiedKit->copyFrom(m_kit);
        m_isDefaultKit = (m_kit == KitManager::instance()->defaultKit());
    } else {
        // This branch will only ever get reached once during setup of widget for a not-yet-existing
        // kit.
        m_isDefaultKit = false;
    }
    m_iconButton->setIcon(m_modifiedKit->icon());
    m_nameEdit->setText(m_modifiedKit->displayName());
    emit dirty();
}

bool KitManagerConfigWidget::isDirty() const
{
    return !m_kit
            || !m_kit->isEqual(m_modifiedKit)
            || m_isDefaultKit != (KitManager::instance()->defaultKit() == m_kit);
}

bool KitManagerConfigWidget::isValid() const
{
    return m_modifiedKit->isValid();
}

QString KitManagerConfigWidget::validityMessage() const
{
    return m_modifiedKit->toHtml();
}

void KitManagerConfigWidget::addConfigWidget(ProjectExplorer::KitConfigWidget *widget)
{
    Q_ASSERT(widget);
    Q_ASSERT(!m_widgets.contains(widget));

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

Kit *KitManagerConfigWidget::workingCopy() const
{
    return m_modifiedKit;
}

bool KitManagerConfigWidget::configures(Kit *k) const
{
    return m_kit == k;
}

void KitManagerConfigWidget::setIsDefaultKit(bool d)
{
    if (m_isDefaultKit != d)
        return;
    m_isDefaultKit = d;
    emit dirty();
}

bool KitManagerConfigWidget::isDefaultKit() const
{
    return m_isDefaultKit;
}

void KitManagerConfigWidget::removeKit()
{
    if (!m_kit)
        return;
    KitManager::instance()->deregisterKit(m_kit);
}

void KitManagerConfigWidget::setIcon()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Select Icon"), m_modifiedKit->iconPath(), tr("Images (*.png *.xpm *.jpg)"));
    if (path.isEmpty())
        return;

    const QIcon icon = QIcon(path);
    if (icon.isNull())
        return;

    m_iconButton->setIcon(icon);
    m_modifiedKit->setIconPath(path);
    emit dirty();
}

void KitManagerConfigWidget::setDisplayName()
{
    m_modifiedKit->setDisplayName(m_nameEdit->text());
}

void KitManagerConfigWidget::workingCopyWasUpdated(Kit *k)
{
    if (k != m_modifiedKit || m_fixingKit)
        return;

    m_fixingKit = true;
    k->fix();
    m_fixingKit = false;

    foreach (KitConfigWidget *w, m_widgets)
        w->refresh();
    m_nameEdit->setText(k->displayName());
    m_iconButton->setIcon(k->icon());
    emit dirty();
}

void KitManagerConfigWidget::kitWasUpdated(Kit *k)
{
    if (m_kit == k)
        discard();
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
