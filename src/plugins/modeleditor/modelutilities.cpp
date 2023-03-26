// Copyright (C) 2018 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelutilities.h"

#include "qmt/model/mobject.h"
#include "qmt/model/mdependency.h"
#include "qmt/model/mpackage.h"

namespace ModelEditor {
namespace Internal {

ModelUtilities::ModelUtilities(QObject *parent)
    : QObject(parent)
{
}

ModelUtilities::~ModelUtilities()
{
}

bool ModelUtilities::haveDependency(const qmt::MObject *source,
                                    const qmt::MObject *target, bool inverted)
{
    qmt::MDependency::Direction aToB = qmt::MDependency::AToB;
    qmt::MDependency::Direction bToA = qmt::MDependency::BToA;
    if (inverted) {
        aToB = qmt::MDependency::BToA;
        bToA = qmt::MDependency::AToB;
    }
    for (const qmt::Handle<qmt::MRelation> &handle : source->relations()) {
        if (auto dependency = dynamic_cast<qmt::MDependency *>(handle.target())) {
            if (dependency->source() == source->uid()
                    && dependency->target() == target->uid()
                    && (dependency->direction() == aToB
                        || dependency->direction() == qmt::MDependency::Bidirectional)) {
                return true;
            }
            if (dependency->source() == target->uid()
                    && dependency->target() == source->uid()
                    && (dependency->direction() == bToA
                        || dependency->direction() == qmt::MDependency::Bidirectional)) {
                return true;
            }
        }
    }
    if (!inverted)
        return haveDependency(target, source, true);
    return false;
}

bool ModelUtilities::haveDependency(const qmt::MObject *source,
                                    const QList<qmt::MPackage *> &targets)
{
    for (const qmt::MPackage *target : targets) {
        if (haveDependency(source, target))
            return true;
    }
    return false;
}

} // namespace Internal
} // namespace ModelEditor
