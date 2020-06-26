/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "kitmanagerconfigwidget.h"

#include "devicesupport/idevicefactory.h"
#include "kit.h"
#include "kitinformation.h"
#include "kitmanager.h"
#include "projectexplorerconstants.h"
#include "task.h"

#include <coreplugin/variablechooser.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
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
    m_modifiedKit(std::make_unique<Kit>(Utils::Id(WORKING_COPY_KIT_ID)))
{
    static auto alignment
            = static_cast<const Qt::Alignment>(style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));

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
    connect(m_fileSystemFriendlyNameLineEdit, &QLineEdit::textChanged,
            this, &KitManagerConfigWidget::setFileSystemFriendlyName);

    auto inner = new QWidget;
    inner->setLayout(m_layout);

    auto mainLayout = new QGridLayout(this);
    mainLayout->setContentsMargins(1, 1, 1, 1);
    mainLayout->addWidget(inner, 0, 0);

    label = createLabel(tr("Name:"), tr("Kit name and icon."));
    m_layout->addWidget(label, 0, LabelColumn, alignment);
    m_iconButton->setToolTip(tr("Kit icon."));
    auto setIconAction = new QAction(tr("Select Icon..."), this);
    m_iconButton->addAction(setIconAction);
    auto resetIconAction = new QAction(tr("Reset to Device Default Icon"), this);
    m_iconButton->addAction(resetIconAction);

    discard();

    connect(m_iconButton, &QAbstractButton::clicked,
            this, &KitManagerConfigWidget::setIcon);
    connect(setIconAction, &QAction::triggered,
            this, &KitManagerConfigWidget::setIcon);
    connect(resetIconAction, &QAction::triggered,
            this, &KitManagerConfigWidget::resetIcon);
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

    for (KitAspect *aspect : KitManager::kitAspects())
        addAspectToWorkingCopy(aspect);

    updateVisibility();

    if (k && k->isAutoDetected())
        makeStickySubWidgetsReadOnly();
    setVisible(false);
}

KitManagerConfigWidget::~KitManagerConfigWidget()
{
    qDeleteAll(m_widgets);
    m_widgets.clear();
    qDeleteAll(m_actions);
    m_actions.clear();

    // Make sure our workingCopy did not get registered somehow:
    QTC_CHECK(!Utils::contains(KitManager::kits(),
                               Utils::equal(&Kit::id, Utils::Id(WORKING_COPY_KIT_ID))));
}

QString KitManagerConfigWidget::displayName() const
{
    if (m_cachedDisplayName.isEmpty())
        m_cachedDisplayName = m_modifiedKit->displayName();
    return m_cachedDisplayName;
}

QIcon KitManagerConfigWidget::displayIcon() const
{
    // Special case: Extra warning if there are no errors but name is not unique.
    if (m_modifiedKit->isValid() && !m_hasUniqueName) {
        static const QIcon warningIcon(Utils::Icons::WARNING.icon());
        return warningIcon;
    }

    return m_modifiedKit->displayIcon();
}

void KitManagerConfigWidget::apply()
{
    // TODO: Rework the mechanism so this won't be necessary.
    const bool wasDefaultKit = m_isDefaultKit;

    const auto copyIntoKit = [this](Kit *k) { k->copyFrom(m_modifiedKit.get()); };
    if (m_kit) {
        copyIntoKit(m_kit);
        KitManager::notifyAboutUpdate(m_kit);
    } else {
        m_isRegistering = true;
        m_kit = KitManager::registerKit(copyIntoKit);
        m_isRegistering = false;
    }
    m_isDefaultKit = wasDefaultKit;
    if (m_isDefaultKit)
        KitManager::setDefaultKit(m_kit);
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
            || !m_kit->isEqual(m_modifiedKit.get())
            || m_isDefaultKit != (KitManager::defaultKit() == m_kit);
}

QString KitManagerConfigWidget::validityMessage() const
{
    Tasks tmp;
    if (!m_hasUniqueName)
        tmp.append(CompileTask(Task::Warning, tr("Display name is not unique.")));

    return m_modifiedKit->toHtml(tmp);
}

void KitManagerConfigWidget::addAspectToWorkingCopy(KitAspect *aspect)
{
    QTC_ASSERT(aspect, return);
    KitAspectWidget *widget = aspect->createConfigWidget(workingCopy());
    QTC_ASSERT(widget, return);
    QTC_ASSERT(!m_widgets.contains(widget), return);

    const QString name = aspect->displayName() + ':';
    QString toolTip = aspect->description();

    auto action = new QAction(tr("Mark as Mutable"), nullptr);
    action->setCheckable(true);
    action->setChecked(workingCopy()->isMutable(aspect->id()));

    action->setEnabled(!widget->isSticky());
    widget->mainWidget()->addAction(action);
    widget->mainWidget()->setContextMenuPolicy(Qt::ActionsContextMenu);
    connect(action, &QAction::toggled, this, [this, aspect, action] {
        workingCopy()->setMutable(aspect->id(), action->isChecked());
        emit dirty();
    });

    m_actions << action;

    int row = m_layout->rowCount();
    m_layout->addWidget(widget->mainWidget(), row, WidgetColumn);
    if (QWidget *button = widget->buttonWidget())
        m_layout->addWidget(button, row, ButtonColumn);

    static auto alignment
        = static_cast<const Qt::Alignment>(style()->styleHint(QStyle::SH_FormLayoutLabelAlignment));
    QLabel *label = createLabel(name, toolTip);
    m_layout->addWidget(label, row, LabelColumn, alignment);
    m_widgets.append(widget);
    m_labels.append(label);
}

void KitManagerConfigWidget::updateVisibility()
{
    int count = m_widgets.count();
    for (int i = 0; i < count; ++i) {
        KitAspectWidget *widget = m_widgets.at(i);
        const bool visible = widget->visibleInKit()
                && !m_modifiedKit->irrelevantAspects().contains(widget->kitInformationId());
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
    foreach (KitAspectWidget *w, m_widgets) {
        if (w->isSticky())
            w->makeReadOnly();
    }
}

Kit *KitManagerConfigWidget::workingCopy() const
{
    return m_modifiedKit.get();
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
    const Utils::Id deviceType = DeviceTypeKitAspect::deviceTypeId(m_modifiedKit.get());
    QList<IDeviceFactory *> allDeviceFactories = IDeviceFactory::allDeviceFactories();
    if (deviceType.isValid()) {
        const auto less = [deviceType](const IDeviceFactory *f1, const IDeviceFactory *f2) {
            if (f1->deviceType() == deviceType)
                return true;
            if (f2->deviceType() == deviceType)
                return false;
            return f1->displayName() < f2->displayName();
        };
        Utils::sort(allDeviceFactories, less);
    }
    QMenu iconMenu;
    for (const IDeviceFactory * const factory : qAsConst(allDeviceFactories)) {
        if (factory->icon().isNull())
            continue;
        iconMenu.addAction(factory->icon(), tr("Default for %1").arg(factory->displayName()),
                           [this, factory] {
            m_iconButton->setIcon(factory->icon());
            m_modifiedKit->setDeviceTypeForIcon(factory->deviceType());
            emit dirty();
        });
    }
    iconMenu.addSeparator();
    iconMenu.addAction(Utils::PathChooser::browseButtonLabel(), [this] {
        const QString path = QFileDialog::getOpenFileName(this, tr("Select Icon"),
                                                          m_modifiedKit->iconPath().toString(),
                                                          tr("Images (*.png *.xpm *.jpg)"));
        if (path.isEmpty())
            return;
        const QIcon icon(path);
        if (icon.isNull())
            return;
        m_iconButton->setIcon(icon);
        m_modifiedKit->setIconPath(Utils::FilePath::fromString(path));
        emit dirty();
    });
    iconMenu.exec(mapToGlobal(m_iconButton->pos()));
}

void KitManagerConfigWidget::resetIcon()
{
    m_modifiedKit->setIconPath(Utils::FilePath());
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
    if (k != m_modifiedKit.get() || m_fixingKit)
        return;

    m_fixingKit = true;
    k->fix();
    m_fixingKit = false;

    foreach (KitAspectWidget *w, m_widgets)
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
    Q_UNUSED(event)
    foreach (KitAspectWidget *widget, m_widgets)
        widget->refresh();
}

QLabel *KitManagerConfigWidget::createLabel(const QString &name, const QString &toolTip)
{
    auto label = new QLabel(name);
    label->setToolTip(toolTip);
    return label;
}

} // namespace Internal
} // namespace ProjectExplorer
