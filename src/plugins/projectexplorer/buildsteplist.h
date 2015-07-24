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

#include "projectconfiguration.h"

#include <QVariantMap>

namespace ProjectExplorer {

class BuildStep;
class Target;

class PROJECTEXPLORER_EXPORT BuildStepList : public ProjectConfiguration
{
    Q_OBJECT

public:
    BuildStepList(QObject *parent, Core::Id id);
    BuildStepList(QObject *parent, BuildStepList *source);
    BuildStepList(QObject *parent, const QVariantMap &data);
    ~BuildStepList() override;

    QList<BuildStep *> steps() const;
    bool isNull() const;
    int count() const;
    bool isEmpty() const;
    bool contains(Core::Id id) const;

    void insertStep(int position, BuildStep *step);
    void appendStep(BuildStep *step) { insertStep(count(), step); }
    bool removeStep(int position);
    void moveStepUp(int position);
    BuildStep *at(int position);

    Target *target() const;

    virtual QVariantMap toMap() const override;
    void cloneSteps(BuildStepList *source);

signals:
    void stepInserted(int position);
    void aboutToRemoveStep(int position);
    void stepRemoved(int position);
    void stepMoved(int from, int to);

protected:
    virtual bool fromMap(const QVariantMap &map) override;

private:
    QList<BuildStep *> m_steps;
    bool m_isNull;
};

} // namespace ProjectExplorer

