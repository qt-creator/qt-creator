// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"

#include <QStringList>
#include <QSharedPointer>

#include <nodemetainfo.h>
#include "itemlibraryinfo.h"

namespace QmlDesigner {

class ModelNode;
class AbstractProperty;
class ItemLibraryInfo;
class ExternalDependenciesInterface;

namespace Internal {
    class MetaInfoPrivate;
    class ModelPrivate;
    class MetaInfoReader;
    class SubComponentManagerPrivate;
    using MetaInfoPrivatePointer = QSharedPointer<MetaInfoPrivate>;
}

QMLDESIGNERCORE_EXPORT bool operator==(const MetaInfo &first, const MetaInfo &second);
QMLDESIGNERCORE_EXPORT bool operator!=(const MetaInfo &first, const MetaInfo &second);


class QMLDESIGNERCORE_EXPORT MetaInfo
{
    friend Internal::MetaInfoPrivate;
    friend Internal::ModelPrivate;
    friend Internal::MetaInfoReader;
    friend Internal::SubComponentManagerPrivate;
    friend QMLDESIGNERCORE_EXPORT bool operator==(const MetaInfo &, const MetaInfo &);

public:
    MetaInfo(const MetaInfo &metaInfo);
    ~MetaInfo();
    MetaInfo& operator=(const MetaInfo &other);

    ItemLibraryInfo *itemLibraryInfo() const;
public:
    static void initializeGlobal(const QStringList &pluginPaths,
                                 const ExternalDependenciesInterface &externalDependencies);

    static void disableParseItemLibraryDescriptionsUgly(); // ugly hack around broken tests

private:
    bool isGlobal() const;
    static MetaInfo global();

private:
    MetaInfo();

    Internal::MetaInfoPrivatePointer m_p;
    static MetaInfo s_global;
    static QStringList s_pluginDirs;
};

} //namespace QmlDesigner
