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

#include <utils/id.h>

#include <QObject>
#include <QVariantMap>

namespace ProjectExplorer {

class BuildStep;
class Target;

class PROJECTEXPLORER_EXPORT BuildStepList : public QObject
{
    Q_OBJECT

public:
    explicit BuildStepList(QObject *parent, Utils::Id id);
    ~BuildStepList() override;

    void clear();

    QList<BuildStep *> steps() const;

    template <class BS> BS *firstOfType() {
        BS *bs = nullptr;
        for (int i = 0; i < count(); ++i) {
            bs = qobject_cast<BS *>(at(i));
            if (bs)
                return bs;
        }
        return nullptr;
    }
    BuildStep *firstStepWithId(Utils::Id id) const;

    int count() const;
    bool isEmpty() const;
    bool contains(Utils::Id id) const;

    void insertStep(int position, BuildStep *step);
    void insertStep(int position, Utils::Id id);
    void appendStep(BuildStep *step) { insertStep(count(), step); }
    void appendStep(Utils::Id stepId) { insertStep(count(), stepId); }

    struct StepCreationInfo {
        Utils::Id stepId;
        std::function<bool(Target *)> condition; // unset counts as unrestricted
    };

    bool removeStep(int position);
    void moveStepUp(int position);
    BuildStep *at(int position);

    Target *target() { return m_target; }

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &map);

    Utils::Id id() const { return m_id; }
    QString displayName() const;

signals:
    void stepInserted(int position);
    void aboutToRemoveStep(int position);
    void stepRemoved(int position);
    void stepMoved(int from, int to);

private:
    Target *m_target;
    Utils::Id m_id;
    QList<BuildStep *> m_steps;
};

} // namespace ProjectExplorer
