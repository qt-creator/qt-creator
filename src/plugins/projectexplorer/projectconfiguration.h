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

class PROJECTEXPLORER_EXPORT ProjectConfiguration : public QObject
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

    template<class Aspect, typename ...Args>
    Aspect *addAspect(Args && ...args)
    {
        return m_aspects.addAspect<Aspect>(std::forward<Args>(args)...);
    }

    const Utils::AspectContainer &aspects() const { return m_aspects; }

    Utils::BaseAspect *aspect(Utils::Id id) const;
    template <typename T> T *aspect() const { return m_aspects.aspect<T>(); }

    Utils::FilePath mapFromBuildDeviceToGlobalPath(const Utils::FilePath &path) const;

signals:
    void displayNameChanged();
    void toolTipChanged();

protected:
    Utils::AspectContainer m_aspects;

private:
    QPointer<Target> m_target;
    const Utils::Id m_id;
    Utils::DisplayName m_displayName;
    QString m_toolTip;
};

// helper function:
PROJECTEXPLORER_EXPORT Utils::Id idFromMap(const QVariantMap &map);

} // namespace ProjectExplorer
