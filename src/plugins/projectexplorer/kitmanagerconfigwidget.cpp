// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "kitmanagerconfigwidget.h"

#include "devicesupport/idevicefactory.h"
#include "kit.h"
#include "kitaspects.h"
#include "kitmanager.h"
#include "projectexplorertr.h"
#include "task.h"

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>
#include <utils/variablechooser.h>

#include <QAction>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QToolButton>
#include <QSizePolicy>

const char WORKING_COPY_KIT_ID[] = "modified kit";

using namespace Utils;

namespace ProjectExplorer::Internal {

KitManagerConfigWidget::KitManagerConfigWidget(Kit *k, bool &isDefaultKit, bool &hasUniqueName) :
    m_iconButton(new QToolButton),
    m_nameEdit(new QLineEdit),
    m_fileSystemFriendlyNameLineEdit(new QLineEdit),
    m_kit(k),
    m_modifiedKit(std::make_unique<Kit>(Id(WORKING_COPY_KIT_ID))),
    m_isDefaultKit(isDefaultKit),
    m_hasUniqueName(hasUniqueName)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    auto label = new QLabel(Tr::tr("Name:"));
    label->setToolTip(Tr::tr("Kit name and icon."));

    QString toolTip =
        Tr::tr("<html><head/><body><p>The name of the kit suitable for generating "
           "directory names. This value is used for the variable <i>%1</i>, "
           "which for example determines the name of the shadow build directory."
           "</p></body></html>").arg(QLatin1String("Kit:FileSystemName"));
    m_fileSystemFriendlyNameLineEdit->setToolTip(toolTip);
    QRegularExpression fileSystemFriendlyNameRegexp(QLatin1String("^[A-Za-z0-9_-]*$"));
    Q_ASSERT(fileSystemFriendlyNameRegexp.isValid());
    m_fileSystemFriendlyNameLineEdit->setValidator(new QRegularExpressionValidator(fileSystemFriendlyNameRegexp, m_fileSystemFriendlyNameLineEdit));

    auto fsLabel = new QLabel(Tr::tr("File system name:"));
    fsLabel->setToolTip(toolTip);
    connect(m_fileSystemFriendlyNameLineEdit, &QLineEdit::textChanged,
            this, &KitManagerConfigWidget::setFileSystemFriendlyName);

    using namespace Layouting;
    Grid page {
        withFormAlignment,
        columnStretch(1, 2),
        label, m_nameEdit, m_iconButton, br,
        fsLabel, m_fileSystemFriendlyNameLineEdit, br,
        noMargin
    };

    m_iconButton->setToolTip(Tr::tr("Kit icon."));
    auto setIconAction = new QAction(Tr::tr("Select Icon..."), this);
    m_iconButton->addAction(setIconAction);
    auto resetIconAction = new QAction(Tr::tr("Reset to Device Default Icon"), this);
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

    auto chooser = new VariableChooser(this);
    chooser->addSupportedWidget(m_nameEdit);
    chooser->addMacroExpanderProvider([this] { return m_modifiedKit->macroExpander(); });

    for (KitAspectFactory *factory : KitManager::kitAspectFactories())
        addAspectToWorkingCopy(page, factory);

    page.attachTo(this);

    updateVisibility();

    if (k && k->isAutoDetected())
        makeStickySubWidgetsReadOnly();
    setVisible(false);
}

KitManagerConfigWidget::~KitManagerConfigWidget()
{
    qDeleteAll(m_kitAspects);
    m_kitAspects.clear();

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
        static const QIcon warningIcon(Icons::WARNING.icon());
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
        tmp.append(CompileTask(Task::Warning, Tr::tr("Display name is not unique.")));

    return m_modifiedKit->toHtml(tmp);
}

void KitManagerConfigWidget::addAspectToWorkingCopy(Layouting::LayoutItem &parent, KitAspectFactory *factory)
{
    QTC_ASSERT(factory, return);
    KitAspect *aspect = factory->createKitAspect(workingCopy());
    QTC_ASSERT(aspect, return);
    QTC_ASSERT(!m_kitAspects.contains(aspect), return);

    aspect->addToLayout(parent);
    m_kitAspects.append(aspect);

    connect(aspect->mutableAction(), &QAction::toggled,
            this, &KitManagerConfigWidget::dirty);
}

void KitManagerConfigWidget::updateVisibility()
{
    for (KitAspect *aspect : std::as_const(m_kitAspects)) {
        const KitAspectFactory *factory = aspect->factory();
        const bool visibleInKit = factory->isApplicableToKit(m_modifiedKit.get());
        const bool irrelevant = m_modifiedKit->irrelevantAspects().contains(factory->id());
        aspect->setVisible(visibleInKit && !irrelevant);
    }
}

void KitManagerConfigWidget::makeStickySubWidgetsReadOnly()
{
    for (KitAspect *aspect : std::as_const(m_kitAspects)) {
        if (aspect->kit()->isSticky(aspect->factory()->id()))
            aspect->makeReadOnly();
    }
}

Kit *KitManagerConfigWidget::workingCopy() const
{
    return m_modifiedKit.get();
}

bool KitManagerConfigWidget::isDefaultKit() const
{
    return m_isDefaultKit;
}

void KitManagerConfigWidget::setIcon()
{
    const Id deviceType = DeviceTypeKitAspect::deviceTypeId(m_modifiedKit.get());
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
    for (const IDeviceFactory * const factory : std::as_const(allDeviceFactories)) {
        if (factory->icon().isNull())
            continue;
        QAction *action = iconMenu.addAction(factory->icon(),
                                             Tr::tr("Default for %1").arg(factory->displayName()),
                                             [this, factory] {
                                                 m_iconButton->setIcon(factory->icon());
                                                 m_modifiedKit->setDeviceTypeForIcon(
                                                     factory->deviceType());
                                                 emit dirty();
                                             });
        action->setIconVisibleInMenu(true);
    }
    iconMenu.addSeparator();
    iconMenu.addAction(PathChooser::browseButtonLabel(), [this] {
        const FilePath path = FileUtils::getOpenFilePath(this, Tr::tr("Select Icon"),
                                                         m_modifiedKit->iconPath(),
                                                         Tr::tr("Images (*.png *.xpm *.jpg)"));
        if (path.isEmpty())
            return;
        const QIcon icon(path.toString());
        if (icon.isNull())
            return;
        m_iconButton->setIcon(icon);
        m_modifiedKit->setIconPath(path);
        emit dirty();
    });
    iconMenu.exec(mapToGlobal(m_iconButton->pos()));
}

void KitManagerConfigWidget::resetIcon()
{
    m_modifiedKit->setIconPath({});
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

    for (KitAspect *w : std::as_const(m_kitAspects))
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
    for (KitAspect *aspect : std::as_const(m_kitAspects))
        aspect->refresh();
}

} // ProjectExplorer::Internal
