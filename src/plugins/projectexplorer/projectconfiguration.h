/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PROJECTCONFIGURATION_H
#define PROJECTCONFIGURATION_H

#include "projectexplorer_export.h"

#include <coreplugin/id.h>
#include <utils/macroexpander.h>

#include <QObject>
#include <QString>
#include <QVariantMap>

namespace ProjectExplorer {

class Project;

class PROJECTEXPLORER_EXPORT ProjectConfiguration : public QObject
{
    Q_OBJECT

public:
    // ctors are protected
    virtual ~ProjectConfiguration();

    Core::Id id() const;
    QString displayName() const;

    bool usesDefaultDisplayName() const;
    void setDisplayName(const QString &name);
    void setDefaultDisplayName(const QString &name);

    // Note: Make sure subclasses call the superclasses' fromMap() function!
    virtual bool fromMap(const QVariantMap &map);

    // Note: Make sure subclasses call the superclasses' toMap() function!
    virtual QVariantMap toMap() const;

    Utils::MacroExpander *macroExpander() { return &m_macroExpander; }
    const Utils::MacroExpander *macroExpander() const { return &m_macroExpander; }

signals:
    void displayNameChanged();

protected:
    ProjectConfiguration(QObject *parent, Core::Id id);
    ProjectConfiguration(QObject *parent, const ProjectConfiguration *source);

private:
    Core::Id m_id;
    QString m_displayName;
    QString m_defaultDisplayName;
    Utils::MacroExpander m_macroExpander;
};

// helper functions:
PROJECTEXPLORER_EXPORT Core::Id idFromMap(const QVariantMap &map);
PROJECTEXPLORER_EXPORT QString displayNameFromMap(const QVariantMap &map);

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::ProjectConfiguration *)

#endif // PROJECTCONFIGURATION_H
