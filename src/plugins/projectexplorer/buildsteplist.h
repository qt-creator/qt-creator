// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    template <class BS> BS *firstOfType() const {
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
    BuildStep *at(int position) const;

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
