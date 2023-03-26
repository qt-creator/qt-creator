// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "createscenecommand.h"

#include <QDebug>

namespace QmlDesigner {

QDebug operator <<(QDebug debug, const CreateSceneCommand &command)
{
    return debug.nospace() << "CreateSceneCommand("
                           << "instances: " << command.instances << ", "
                           << "reparentInstances: " << command.reparentInstances << ", "
                           << "ids: " << command.ids << ", "
                           << "valueChanges: " << command.valueChanges << ", "
                           << "bindingChanges: " << command.bindingChanges << ", "
                           << "auxiliaryChanges: " << command.auxiliaryChanges << ", "
                           << "imports: " << command.imports << ", "
                           << "mockupTypes: " << command.mockupTypes << ", "
                           << "fileUrl: " << command.fileUrl << ", "
                           << "resourceUrl: " << command.resourceUrl << ", "
                           << "edit3dToolStates: " << command.edit3dToolStates << ", "
                           << "language: " << command.language << ")";
}

}
