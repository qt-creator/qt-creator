/****************************************************************************
**
** Copyright (C) 2018 Jochen Becher
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
    foreach (const qmt::MPackage *target, targets) {
        if (haveDependency(source, target))
            return true;
    }
    return false;
}

} // namespace Internal
} // namespace ModelEditor
