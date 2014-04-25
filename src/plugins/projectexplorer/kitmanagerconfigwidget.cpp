/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#include "projectexplorerconstants.h"

#include "kit.h"
#include "kitmanager.h"

#include <utils/detailswidget.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QRegExp>
#include <QRegExpValidator>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QToolButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStyle>

static const char WORKING_COPY_KIT_ID[] = "modified kit";

namespace ProjectExplorer {
namespace Internal {

KitManagerConfigWidget::KitManagerConfigWidget(Kit *k) :
    m_layout(new QGridLayout),
    m_iconButton(new QToolButton),
    m_nameEdit(new QLineEdit),
    m_fileSystemFriendlyNameLineEdit(new QLineEdit),
    m_kit(k),
    m_modifiedKit(new Kit(Core::Id(WORKING_COPY_KIT_ID))),
    m_fixingKit(false)
{
    static const Qt::Alignment alignment
            = static_cast<Qt::Alignment>(style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));

    m_layout->addWidget(m_nameEdit, 0, WidgetColumn);
    m_layout->addWidget(m_iconButton, 0, ButtonColumn);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QString toolTip =
        tr("<html><head/><body><p>The name of the kit suitable for generating "
           "directory names. This value is used for the variable <i>%1</i>, "
           "which for example determines the name of the shadow build directory."
           "</p></body></html>").arg(QLatin1String(Constants::VAR_CURRENTKIT_FILESYSTEMNAME));
    QLabel *label = createLabel(tr("File system name:"), toolTip);
    m_layout->addWidget(label, 1, LabelColumn, alignment);
    m_fileSystemFriendlyNameLineEdit->setToolTip(toolTip);
    QRegExp fileSystemFriendlyNameRegexp(QLatin1String("^[A-Za-z0-9_-]*$"));
    Q_ASSERT(fileSystemFriendlyNameRegexp.isValid());
    m_fileSystemFriendlyNameLineEdit->setValidator(new QRegExpValidator(fileSystemFriendlyNameRegexp, m_fileSystemFriendlyNameLineEdit));
    m_layout->addWidget(m_fileSystemFriendlyNameLineEdit, 1, WidgetColumn);
    connect(m_fileSystemFriendlyNameLineEdit, SIGNAL(textChanged(QString)), this, SLOT(setFileSystemFriendlyName()));

    QWidget *inner = new QWidget;
    inner->setLayout(m_layout);

    QScrollArea *scroll = new QScrollArea;
    scroll->setWidget(inner);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setFocusPolicy(Qt::NoFocus);

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setMargin(1);
    mainLayout->addWidget(scroll, 0, 0);

    toolTip = tr("Kit name and icon.");
    label = createLabel(tr("Name:"), toolTip);
    m_layout->addWidget(label, 0, LabelColumn, alignment);
    m_iconButton->setToolTip(toolTip);

    discard();

    connect(m_iconButton, SIGNAL(clicked()), this, SLOT(setIcon()));
    connect(m_nameEdit, SIGNAL(textChanged(QString)), this, SLOT(setDisplayName()));

    QObject *km = KitManager::instance();
    connect(km, SIGNAL(unmanagedKitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(workingCopyWasUpdated(ProjectExplorer::Kit*)));
    connect(km, SIGNAL(kitUpdated(ProjectExplorer::Kit*)),
            this, SLOT(kitWasUpdated(ProjectExplorer::Kit*)));
}

KitManagerConfigWidget::~KitManagerConfigWidget()
{
    qDeleteAll(m_widgets);
    m_widgets.clear();
    qDeleteAll(m_actions);
    m_actions.clear();

    KitManager::deleteKit(m_modifiedKit);
    // Make sure our workingCopy did not get registered somehow:
    foreach (const Kit *k, KitManager::kits())
        QTC_CHECK(k->id() != Core::Id(WORKING_COPY_KIT_ID));
}

QString KitManagerConfigWidget::displayName() const
{
    return m_nameEdit->text();
}

void KitManagerConfigWidget::apply()
{
    bool mustSetDefault = m_isDefaultKit;
    bool mustRegister = false;
    if (!m_kit) {
        mustRegister = true;
        m_kit = new Kit;
    }
    m_kit->copyFrom(m_modifiedKit);//m_isDefaultKit is reset in discard() here.
    if (mustRegister)
        KitManager::registerKit(m_kit);

    if (mustSetDefault)
        KitManager::setDefaultKit(m_kit);

    m_isDefaultKit = mustSetDefault;
    emit dirty();
}

void KitManagerConfigWidget::discard()
{
    if (m_kit) {
        m_modifiedKit->copyFrom(m_kit);
        m_isDefaultKit = (m_kit == KitManager::defaultKit());
    } else {
        // This branch will only ever get reached once during setup of widget for a not-yet-existing
        // kit.
        m_isDefaultKit = false;
    }
    m_iconButton->setIcon(m_modifiedKit->icon());
    m_nameEdit->setText(m_modifiedKit->displayName());
    m_fileSystemFriendlyNameLineEdit->setText(m_modifiedKit->customFileSystemFriendlyName());
    emit dirty();
}

bool KitManagerConfigWidget::isDirty() const
{
    return !m_kit
            || !m_kit->isEqual(m_modifiedKit)
            || m_isDefaultKit != (KitManager::defaultKit() == m_kit);
}

bool KitManagerConfigWidget::isValid() const
{
    return m_modifiedKit->isValid();
}

bool KitManagerConfigWidget::hasWarning() const
{
    return m_modifiedKit->hasWarning();
}

QString KitManagerConfigWidget::validityMessage() const
{
    return m_modifiedKit->toHtml();
}

void KitManagerConfigWidget::addConfigWidget(ProjectExplorer::KitConfigWidget *widget)
{
    QTC_ASSERT(widget, return);
    QTC_ASSERT(!m_widgets.contains(widget), return);

    QString name = widget->displayName();
    QString toolTip = widget->toolTip();

    QAction *action = new QAction(tr("Mark as Mutable"), 0);
    action->setCheckable(true);
    action->setData(QVariant::fromValue(qobject_cast<QObject *>(widget)));
    action->setChecked(widget->isMutable());
    action->setEnabled(!widget->isSticky());
    widget->mainWidget()->addAction(action);
    widget->mainWidget()->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(action, SIGNAL(toggled(bool)), this, SLOT(updateMutableState()));
    m_actions << action;

    int row = m_layout->rowCount();
    m_layout->addWidget(widget->mainWidget(), row, WidgetColumn);
    if (QWidget *button = widget->buttonWidget())
        m_layout->addWidget(button, row, ButtonColumn);

    static const Qt::Alignment alignment
        = static_cast<Qt::Alignment>(style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
    QLabel *label = createLabel(name, toolTip);
    m_layout->addWidget(label, row, LabelColumn, alignment);
    m_widgets.append(widget);
    m_labels.append(label);
}

void KitManagerConfigWidget::updateVisibility()
{
    int count = m_widgets.count();
    for (int i = 0; i < count; ++i) {
        KitConfigWidget *widget = m_widgets.at(i);
        bool visible = widget->visibleInKit();
        widget->mainWidget()->setVisible(visible);
        if (widget->buttonWidget())
            widget->buttonWidget()->setVisible(visible);
        m_labels.at(i)->setVisible(visible);
    }
}

void KitManagerConfigWidget::makeStickySubWidgetsReadOnly()
{
    foreach (KitConfigWidget *w, m_widgets) {
        if (w->isSticky())
            w->makeReadOnly();
    }
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
    if (m_isDefaultKit == d)
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
    KitManager::deregisterKit(m_kit);
}

void KitManagerConfigWidget::setIcon()
{
    const Utils::FileName path = Utils::FileName::fromString(
                QFileDialog::getOpenFileName(this, tr("Select Icon"),
                                             m_modifiedKit->iconPath().toString(),
                                             tr("Images (*.png *.xpm *.jpg)")));
    if (path.isEmpty())
        return;

    const QIcon icon = Kit::icon(path);
    if (icon.isNull())
        return;

    m_iconButton->setIcon(icon);
    m_modifiedKit->setIconPath(path);
    emit dirty();
}

void KitManagerConfigWidget::setDisplayName()
{
    int pos = m_nameEdit->cursorPosition();
    m_modifiedKit->setDisplayName(m_nameEdit->text());
    m_nameEdit->setCursorPosition(pos);
}

void KitManagerConfigWidget::setFileSystemFriendlyName()
{
    const int pos = m_fileSystemFriendlyNameLineEdit->cursorPosition();
    m_modifiedKit->setCustomFileSystemFriendlyName(m_fileSystemFriendlyNameLineEdit->text());
    m_fileSystemFriendlyNameLineEdit->setCursorPosition(pos);
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
    m_fileSystemFriendlyNameLineEdit->setText(k->customFileSystemFriendlyName());
    m_iconButton->setIcon(k->icon());
    updateVisibility();
    emit dirty();
}

void KitManagerConfigWidget::kitWasUpdated(Kit *k)
{
    if (m_kit == k)
        discard();
    updateVisibility();
}

void KitManagerConfigWidget::updateMutableState()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action)
        return;
    KitConfigWidget *widget = qobject_cast<KitConfigWidget *>(action->data().value<QObject *>());
    if (!widget)
        return;
    widget->setMutable(action->isChecked());
    emit dirty();
}

QLabel *KitManagerConfigWidget::createLabel(const QString &name, const QString &toolTip)
{
    QLabel *label = new QLabel(name);
    label->setToolTip(toolTip);
    return label;
}

void KitManagerConfigWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if (m_background.size() != size())
        m_background = Utils::DetailsWidget::createBackground(size(), 0, this);
    p.drawPixmap(rect(), m_background);
}

} // namespace Internal
} // namespace ProjectExplorer
