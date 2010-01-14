/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef PROJECTCONFIGURATION_H
#define PROJECTCONFIGURATION_H

#include "projectexplorer_export.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariantMap>

namespace ProjectExplorer {

class Project;

class PROJECTEXPLORER_EXPORT ProjectConfiguration : public QObject
{
    Q_OBJECT

public:
    // ctors are protected
    virtual ~ProjectConfiguration();

    QString id() const;
    QString displayName() const;
    void setDisplayName(const QString &name);

    // Note: Make sure subclasses call the superclasses toMap() method!
    virtual QVariantMap toMap() const;

signals:
    void displayNameChanged();

protected:
    ProjectConfiguration(const QString &id);
    ProjectConfiguration(ProjectConfiguration *config);

    // Note: Make sure subclasses call the superclasses toMap() method!
    virtual bool fromMap(const QVariantMap &map);

private:
    QString m_id;
    QString m_displayName;
};

// helper functions:
PROJECTEXPLORER_EXPORT QString idFromMap(const QVariantMap &map);
PROJECTEXPLORER_EXPORT QString displayNameFromMap(const QVariantMap &map);

} // namespace ProjectExplorer

#endif // PROJECTCONFIGURATION_H
