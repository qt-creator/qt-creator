/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PROJECTEXPLORER_BUILDSTEPLIST_H
#define PROJECTEXPLORER_BUILDSTEPLIST_H

#include "projectexplorer_export.h"

#include "projectconfiguration.h"

#include <QtCore/QVariantMap>

namespace ProjectExplorer {

class BuildStep;
class Target;

class PROJECTEXPLORER_EXPORT BuildStepList : public ProjectConfiguration
{
    Q_OBJECT

public:
    BuildStepList(QObject *parent, const QString &id);
    BuildStepList(QObject *parent, BuildStepList *source);
    BuildStepList(QObject *parent, const QVariantMap &data);
    virtual ~BuildStepList();

    QList<BuildStep *> steps() const;
    bool isNull() const;
    int count() const;
    bool isEmpty() const;
    bool contains(const QString &id) const;

    void insertStep(int position, BuildStep *step);
    bool removeStep(int position);
    void moveStepUp(int position);
    BuildStep *at(int position);

    Target *target() const;

    virtual QVariantMap toMap() const;
    void cloneSteps(BuildStepList *source);

signals:
    void stepInserted(int position);
    void aboutToRemoveStep(int position);
    void stepRemoved(int position);
    void stepMoved(int from, int to);

protected:
    virtual bool fromMap(const QVariantMap &map);

private:
    QList<BuildStep *> m_steps;
    bool m_isNull;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::BuildStepList *)

#endif // PROJECTEXPLORER_BUILDSTEPLIST_H
