// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/aspects.h>
#include <utils/displayname.h>
#include <utils/id.h>
#include <utils/store.h>

#include <QPointer>
#include <QWidget>

namespace ProjectExplorer {

class Kit;
class Project;
class Target;

class PROJECTEXPLORER_EXPORT ProjectConfiguration : public Utils::AspectContainer
{
    Q_OBJECT

protected:
    explicit ProjectConfiguration(Target *target, Utils::Id id);

public:
    ~ProjectConfiguration() override;

    Utils::Id id() const;

    QString displayName() const { return m_displayName.value(); }
    QString expandedDisplayName() const;
    bool usesDefaultDisplayName() const { return m_displayName.usesDefaultValue(); }
    void setDisplayName(const QString &name);
    void setDefaultDisplayName(const QString &name);
    void forceDisplayNameSerialization() { m_displayName.forceSerialization(); }

    void setToolTip(const QString &text);
    QString toolTip() const;

    void reportError() { m_hasError = true; }
    bool hasError() const { return m_hasError; }

    // Note: Make sure subclasses call the superclasses' fromMap() function!
    virtual void fromMap(const Utils::Store &map) override;
    // Note: Make sure subclasses call the superclasses' toMap() function!
    virtual void toMap(Utils::Store &map) const override;

    Target *target() const;
    Project *project() const;
    Kit *kit() const;

    static Utils::Key settingsIdKey();

signals:
    void displayNameChanged();
    void toolTipChanged();

private:
    QPointer<Target> m_target;
    const Utils::Id m_id;
    Utils::DisplayName m_displayName;
    QString m_toolTip;
    bool m_hasError = false;
};

// helper function:
PROJECTEXPLORER_EXPORT Utils::Id idFromMap(const Utils::Store &map);

} // namespace ProjectExplorer
