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
    ~BuildStepList() override;

    QList<BuildStep *> steps() const;
    QList<BuildStep *> steps(const std::function<bool(const BuildStep *)> &filter) const;
    template <class BS> BS *firstOfType() {
        BS *bs = nullptr;
        for (int i = 0; i < count(); ++i) {
            bs = qobject_cast<BS *>(at(i));
            if (bs)
                return bs;
        }
        return nullptr;
    }
    template <class BS> QList<BS *>allOfType() {
        QList<BS *> result;
        BS *bs = nullptr;
        for (int i = 0; i < count(); ++i) {
            bs = qobject_cast<BS *>(at(i));
            if (bs)
                result.append(bs);
        }
        return result;
    }

    int count() const;
    bool isEmpty() const;
    bool contains(Core::Id id) const;

    void insertStep(int position, BuildStep *step);
    void appendStep(BuildStep *step) { insertStep(count(), step); }
    bool removeStep(int position);
    void moveStepUp(int position);
    BuildStep *at(int position);

    Target *target() const;
    Project *project() const override;

    virtual QVariantMap toMap() const override;
    virtual bool fromMap(const QVariantMap &map) override;
    void cloneSteps(BuildStepList *source);

    bool isActive() const override;

signals:
    void stepInserted(int position);
    void aboutToRemoveStep(int position);
    void stepRemoved(int position);
    void stepMoved(int from, int to);

private:
    QList<BuildStep *> m_steps;
};

} // namespace ProjectExplorer
