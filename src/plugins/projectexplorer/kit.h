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

#pragma once

#include "projectexplorer_export.h"
#include "task.h"

#include <coreplugin/featureprovider.h>

#include <QSet>
#include <QVariant>

namespace Utils {
class Environment;
class MacroExpander;
} // namespace Utils

namespace ProjectExplorer {
class IOutputParser;

namespace Internal {
class KitManagerPrivate;
class KitModel;
class KitPrivate;
} // namespace Internal

/**
 * @brief The Kit class
 *
 * The kit holds a set of values defining a system targeted by the software
 * under development.
 */
class PROJECTEXPLORER_EXPORT Kit
{
public:
    using Predicate = std::function<bool(const Kit *)>;

    explicit Kit(Core::Id id = Core::Id());

    // Do not trigger evaluations
    void blockNotification();
    // Trigger evaluations again.
    void unblockNotification();

    bool isValid() const;
    bool hasWarning() const;
    QList<Task> validate() const;
    void fix(); // Fix the individual kit information: Make sure it contains a valid value.
                // Fix will not look at other information in the kit!
    void setup(); // Apply advanced magic(TM). Used only once on each kit during initial setup.
    void upgrade(); // Upgrade settings to new syntax (if appropriate).

    QString unexpandedDisplayName() const;
    QString displayName() const;
    void setUnexpandedDisplayName(const QString &name);

    QString fileSystemFriendlyName() const;
    QString customFileSystemFriendlyName() const;
    void setCustomFileSystemFriendlyName(const QString &fileSystemFriendlyName);

    bool isAutoDetected() const;
    QString autoDetectionSource() const;
    bool isSdkProvided() const;
    Core::Id id() const;

    QIcon icon() const;
    Utils::FileName iconPath() const;
    void setIconPath(const Utils::FileName &path);

    QList<Core::Id> allKeys() const;
    QVariant value(Core::Id key, const QVariant &unset = QVariant()) const;
    bool hasValue(Core::Id key) const;
    void setValue(Core::Id key, const QVariant &value);
    void setValueSilently(Core::Id key, const QVariant &value);
    void removeKey(Core::Id key);
    void removeKeySilently(Core::Id key);
    bool isSticky(Core::Id id) const;

    bool isDataEqual(const Kit *other) const;
    bool isEqual(const Kit *other) const;

    void addToEnvironment(Utils::Environment &env) const;
    IOutputParser *createOutputParser() const;

    QString toHtml(const QList<Task> &additional = QList<Task>()) const;
    Kit *clone(bool keepName = false) const;
    void copyFrom(const Kit *k);

    // Note: Stickyness is *not* saved!
    void setAutoDetected(bool detected);
    void setAutoDetectionSource(const QString &autoDetectionSource);
    void makeSticky();
    void setSticky(Core::Id id, bool b);
    void makeUnSticky();

    void setMutable(Core::Id id, bool b);
    bool isMutable(Core::Id id) const;

    QSet<Core::Id> supportedPlatforms() const;
    QSet<Core::Id> availableFeatures() const;
    bool hasFeatures(const QSet<Core::Id> &features) const;
    Utils::MacroExpander *macroExpander() const;

private:
    void setSdkProvided(bool sdkProvided);

    ~Kit();
    Kit(const QVariantMap &data);

    // Unimplemented.
    Kit(const Kit &other);
    void operator=(const Kit &other);

    void kitDisplayNameChanged();
    void kitUpdated();

    QVariantMap toMap() const;

    Internal::KitPrivate *const d;

    friend class KitInformation;
    friend class KitManager;
    friend class Internal::KitManagerPrivate;
    friend class Internal::KitModel; // needed for setAutoDetected() when cloning kits
};

class KitGuard
{
public:
    KitGuard(Kit *k) : m_kit(k)
    { k->blockNotification(); }

    ~KitGuard() { m_kit->unblockNotification(); }
private:
    Kit *m_kit;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Kit *)
