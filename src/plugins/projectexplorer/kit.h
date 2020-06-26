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

#include <memory>

namespace Utils {
class Environment;
class MacroExpander;
class OutputLineParser;
} // namespace Utils

namespace ProjectExplorer {

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
    static Predicate defaultPredicate();

    explicit Kit(Utils::Id id = Utils::Id());
    explicit Kit(const QVariantMap &data);
    ~Kit();

    // Do not trigger evaluations
    void blockNotification();
    // Trigger evaluations again.
    void unblockNotification();

    bool isValid() const;
    bool hasWarning() const;
    Tasks validate() const;
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
    Utils::Id id() const;

    // The higher the weight, the more aspects have sensible values for this kit.
    // For instance, a kit where a matching debugger was found for the toolchain will have a
    // higher weight than one whose toolchain does not match a known debugger, assuming
    // all other aspects are equal.
    int weight() const;

    QIcon icon() const; // Raw device icon, independent of warning or error.
    QIcon displayIcon() const; // Error or warning or device icon.
    Utils::FilePath iconPath() const;
    void setIconPath(const Utils::FilePath &path);
    void setDeviceTypeForIcon(Utils::Id deviceType);

    QList<Utils::Id> allKeys() const;
    QVariant value(Utils::Id key, const QVariant &unset = QVariant()) const;
    bool hasValue(Utils::Id key) const;
    void setValue(Utils::Id key, const QVariant &value);
    void setValueSilently(Utils::Id key, const QVariant &value);
    void removeKey(Utils::Id key);
    void removeKeySilently(Utils::Id key);
    bool isSticky(Utils::Id id) const;

    bool isDataEqual(const Kit *other) const;
    bool isEqual(const Kit *other) const;

    void addToEnvironment(Utils::Environment &env) const;
    QList<Utils::OutputLineParser *> createOutputParsers() const;

    QString toHtml(const Tasks &additional = Tasks(), const QString &extraText = QString()) const;
    Kit *clone(bool keepName = false) const;
    void copyFrom(const Kit *k);

    // Note: Stickyness is *not* saved!
    void setAutoDetected(bool detected);
    void setAutoDetectionSource(const QString &autoDetectionSource);
    void makeSticky();
    void setSticky(Utils::Id id, bool b);
    void makeUnSticky();

    void setMutable(Utils::Id id, bool b);
    bool isMutable(Utils::Id id) const;

    void setIrrelevantAspects(const QSet<Utils::Id> &irrelevant);
    QSet<Utils::Id> irrelevantAspects() const;

    QSet<Utils::Id> supportedPlatforms() const;
    QSet<Utils::Id> availableFeatures() const;
    bool hasFeatures(const QSet<Utils::Id> &features) const;
    Utils::MacroExpander *macroExpander() const;

    QString newKitName(const QList<Kit *> &allKits) const;
    static QString newKitName(const QString &name, const QList<Kit *> &allKits);

private:
    static void copyKitCommon(Kit *target, const Kit *source);
    void setSdkProvided(bool sdkProvided);

    Kit(const Kit &other) = delete;
    void operator=(const Kit &other) = delete;

    void kitUpdated();

    QVariantMap toMap() const;

    const std::unique_ptr<Internal::KitPrivate> d;

    friend class KitAspect;
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

using TasksGenerator = std::function<Tasks(const Kit *)>;

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Kit *)
