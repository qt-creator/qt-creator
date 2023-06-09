// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/aspects.h>
#include <utils/displayname.h>
#include <utils/id.h>

#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariantMap>
#include <QWidget>

namespace ProjectExplorer {

class Kit;
class Project;
class Target;

class PROJECTEXPLORER_EXPORT ProjectConfiguration : public Utils::AspectContainer
{
    Q_OBJECT

protected:
    explicit ProjectConfiguration(QObject *parent, Utils::Id id);

public:
    ~ProjectConfiguration() override;

    Utils::Id id() const;

    QString displayName() const { return m_displayName.value(); }
    QString expandedDisplayName() const;
    bool usesDefaultDisplayName() const { return m_displayName.usesDefaultValue(); }
    void setDisplayName(const QString &name);
    void setDefaultDisplayName(const QString &name);

    void setToolTip(const QString &text);
    QString toolTip() const;

    // Note: Make sure subclasses call the superclasses' fromMap() function!
    virtual bool fromMap(const QVariantMap &map);

    // Note: Make sure subclasses call the superclasses' toMap() function!
    virtual QVariantMap toMap() const;

    Target *target() const;
    Project *project() const;
    Kit *kit() const;

    static QString settingsIdKey();

signals:
    void displayNameChanged();
    void toolTipChanged();

private:
    QPointer<Target> m_target;
    const Utils::Id m_id;
    Utils::DisplayName m_displayName;
    QString m_toolTip;
};

// helper function:
PROJECTEXPLORER_EXPORT Utils::Id idFromMap(const QVariantMap &map);

} // namespace ProjectExplorer
