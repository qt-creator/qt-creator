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

    const Utils::BaseAspects &aspects() const { return m_aspects; }

    Utils::BaseAspect *aspect(Utils::Id id) const;
    template <typename T> T *aspect() const { return m_aspects.aspect<T>(); }

    void acquaintAspects();

signals:
    void displayNameChanged();
    void toolTipChanged();

protected:
    Utils::BaseAspects m_aspects;

private:
    QPointer<Target> m_target;
    const Utils::Id m_id;
    Utils::DisplayName m_displayName;
    QString m_toolTip;
};

// helper function:
PROJECTEXPLORER_EXPORT Utils::Id idFromMap(const QVariantMap &map);

} // namespace ProjectExplorer
