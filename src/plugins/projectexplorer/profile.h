/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef PROFILE_H
#define PROFILE_H

#include "projectexplorer_export.h"
#include "task.h"

#include <coreplugin/id.h>

#include <QVariant>

namespace Utils { class Environment; }

namespace ProjectExplorer {

namespace Internal {
class ProfileManagerPrivate;
class ProfilePrivate;
} // namespace Internal

/**
 * @brief The Profile class
 *
 * The profile holds a set of values defining a system targeted by the software
 * under development.
 */
class PROJECTEXPLORER_EXPORT Profile
{
public:
    Profile();
    ~Profile();

    bool isValid() const;
    QList<Task> validate();

    QString displayName() const;
    void setDisplayName(const QString &name);

    bool isAutoDetected() const;
    Core::Id id() const;

    QIcon icon() const;
    QString iconPath() const;
    void setIconPath(const QString &path);

    QVariant value(const Core::Id &key, const QVariant &unset = QVariant()) const;
    bool hasValue(const Core::Id &key) const;
    void setValue(const Core::Id &key, const QVariant &value);
    void removeKey(const Core::Id &key);

    bool operator==(const Profile &other) const;

    void addToEnvironment(Utils::Environment &env) const;

    QString toHtml();
    Profile *clone(bool keepName = false) const;

private:
    // Unimplemented.
    Profile(const Profile &other);
    void operator=(const Profile &other);

    void setAutoDetected(bool detected);
    void setValid(bool valid);

    void profileUpdated();

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &value);

    Internal::ProfilePrivate *d;

    friend class ProfileManager;
    friend class Internal::ProfileManagerPrivate;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Profile *)

#endif // PROFILE_H
