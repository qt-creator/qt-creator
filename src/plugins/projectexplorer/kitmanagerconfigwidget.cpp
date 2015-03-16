/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "kitmanagerconfigwidget.h"
#include "projectexplorerconstants.h"

#include "kit.h"
#include "kitmanager.h"
#include "task.h"

#include <coreplugin/variablechooser.h>

#include <utils/detailswidget.h>
#include <utils/qtcassert.h>
#include <utils/macroexpander.h>

#include <QAction>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
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
    m_fixingKit(false),
    m_hasUniqueName(true)
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
    QRegularExpression fileSystemFriendlyNameRegexp(QLatin1String("^[A-Za-z0-9_-]*$"));
    Q_ASSERT(fileSystemFriendlyNameRegexp.isValid());
    m_fileSystemFriendlyNameLineEdit->setValidator(new QRegularExpressionValidator(fileSystemFriendlyNameRegexp, m_fileSystemFriendlyNameLineEdit));
    m_layout->addWidget(m_fileSystemFriendlyNameLineEdit, 1, WidgetColumn);
    connect(m_fileSystemFriendlyNameLineEdit, &QLineEdit::textChanged, this, &KitManagerConfigWidget::setFileSystemFriendlyName);

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

    connect(m_iconButton, &QAbstractButton::clicked,
            this, &KitManagerConfigWidget::setIcon);
    connect(m_nameEdit, &QLineEdit::textChanged,
            this, &KitManagerConfigWidget::setDisplayName);

    KitManager *km = KitManager::instance();
    connect(km, &KitManager::unmanagedKitUpdated,
            this, &KitManagerConfigWidget::workingCopyWasUpdated);
    connect(km, &KitManager::kitUpdated,
            this, &KitManagerConfigWidget::kitWasUpdated);

    auto chooser = new Core::VariableChooser(this);
    chooser->addSupportedWidget(m_nameEdit);
    chooser->addMacroExpanderProvider([this]() { return m_modifiedKit->macroExpander(); });
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
    if (m_cachedDisplayName.isEmpty())
        m_cachedDisplayName = m_modifiedKit->displayName();
    return m_cachedDisplayName;
}

void KitManagerConfigWidget::apply()
{
    bool mustSetDefault = m_isDefaultKit;
    bool mustRegister = false;
    if (!m_kit) {
        mustRegister = true;
        m_kit = new Kit;
    }
    m_kit->copyFrom(m_modifiedKit); //m_isDefaultKit is reset in discard() here.
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
    m_nameEdit->setText(m_modifiedKit->unexpandedDisplayName());
    m_cachedDisplayName.clear();
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
    return m_modifiedKit->hasWarning() || !m_hasUniqueName;
}

QString KitManagerConfigWidget::validityMessage() const
{
    QList<Task> tmp;
    if (!m_hasUniqueName) {
        tmp.append(Task(Task::Warning, tr("Display name is not unique."), Utils::FileName(), -1,
                        ProjectExplorer::Constants::TASK_CATEGORY_COMPILE));
    }
    return m_modifiedKit->toHtml(tmp);
}

void KitManagerConfigWidget::addConfigWidget(KitConfigWidget *widget)
{
    QTC_ASSERT(widget, return);
    QTC_ASSERT(!m_widgets.contains(widget), return);

    QString name = widget->displayName();
    QString toolTip = widget->toolTip();

    QAction *action = new QAction(tr("Mark as Mutable"), 0);
    action->setCheckable(true);
    action->setChecked(widget->isMutable());
    action->setEnabled(!widget->isSticky());
    widget->mainWidget()->addAction(action);
    widget->mainWidget()->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(action, &QAction::toggled, this, [this, widget, action] {
        widget->setMutable(action->isChecked());
        emit dirty();
    });

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

void KitManagerConfigWidget::setHasUniqueName(bool unique)
{
    m_hasUniqueName = unique;
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
    m_cachedDisplayName.clear();
    m_modifiedKit->setUnexpandedDisplayName(m_nameEdit->text());
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

    m_cachedDisplayName.clear();

    if (k->unexpandedDisplayName() != m_nameEdit->text())
        m_nameEdit->setText(k->unexpandedDisplayName());

    m_fileSystemFriendlyNameLineEdit->setText(k->customFileSystemFriendlyName());
    m_iconButton->setIcon(k->icon());
    updateVisibility();
    emit dirty();
}

void KitManagerConfigWidget::kitWasUpdated(Kit *k)
{
    if (m_kit == k) {
        bool emitSignal = m_kit->isAutoDetected() != m_modifiedKit->isAutoDetected();
        discard();
        if (emitSignal)
            emit isAutoDetectedChanged();
    }
    updateVisibility();
}

void KitManagerConfigWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    foreach (KitConfigWidget *widget, m_widgets)
        widget->refresh();
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
