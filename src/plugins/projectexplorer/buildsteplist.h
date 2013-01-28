/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PROJECTEXPLORER_BUILDSTEPLIST_H
#define PROJECTEXPLORER_BUILDSTEPLIST_H

#include "projectexplorer_export.h"

#include "projectconfiguration.h"

#include <QVariantMap>

namespace ProjectExplorer {

class BuildStep;
class Target;

class PROJECTEXPLORER_EXPORT BuildStepList : public ProjectConfiguration
{
    Q_OBJECT

public:
    BuildStepList(QObject *parent, const Core::Id id);
    BuildStepList(QObject *parent, BuildStepList *source);
    BuildStepList(QObject *parent, const QVariantMap &data);
    virtual ~BuildStepList();

    QList<BuildStep *> steps() const;
    bool isNull() const;
    int count() const;
    bool isEmpty() const;
    bool contains(const Core::Id id) const;

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
