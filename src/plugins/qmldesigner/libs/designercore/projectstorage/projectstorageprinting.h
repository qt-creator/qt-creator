// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectstoragetypes.h"

#include <utils/smallstringio.h>

namespace QmlDesigner {

template<auto Type, typename InternalIntergerType = long long>
inline QDebug operator<<(QDebug debug, BasicId<Type, InternalIntergerType> id)
{
    debug.noquote() << "(" << id.id << ")";

    return debug;
}

} // namespace QmlDesigner

namespace QmlDesigner::Storage {

inline QDebug operator<<(QDebug debug, const Version &version)
{
    debug.noquote() << "(" << version.major.value << ", " << version.minor.value << ")";

    return debug;
}

inline QDebug operator<<(QDebug debug, const Import &import)
{
    debug.noquote() << "(" << import.moduleId << ", " << import.sourceId << ", " << import.version
                    << ")";

    return debug;
}

inline QDebug operator<<(QDebug debug, const ExportedType &type)
{
    debug.noquote() << "(" << type.name << ", " << type.version << ")";

    return debug;
}

inline QDebug operator<<(QDebug debug, const QualifiedImportedType &type)
{
    return debug.noquote() << "(" << type.name << ", " << type.import << ")";
}

inline QDebug operator<<(QDebug debug, const ImportedType &type)
{
    return debug.noquote() << "(" << type.name << ")";
}

inline QDebug operator<<(QDebug debug, const ImportedTypeName &importedTypeName)
{
    std::visit([&](auto &&type) { debug << type; }, importedTypeName);

    return debug;
}

inline QDebug operator<<(QDebug debug, const Type &type)
{
    debug.noquote() << "(" << type.typeName << ", " << type.prototype << ", " << type.exportedTypes
                    << ")";

    return debug;
}

} // namespace QmlDesigner::Storage
