// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljs_global.h"
#include <cplusplus/CppDocument.h>
#include <languageutils/fakemetaobject.h>

#include <QCoreApplication>
#include <QHash>

namespace QmlJS {

class QMLJS_EXPORT FindExportedCppTypes
{
public:
    FindExportedCppTypes(const CPlusPlus::Snapshot &snapshot);

    // document must have a valid source and ast for the duration of the call
    QStringList operator()(const CPlusPlus::Document::Ptr &document);

    QList<LanguageUtils::FakeMetaObject::ConstPtr> exportedTypes() const;
    QHash<QString, QString> contextProperties() const;

    static bool maybeExportsTypes(const CPlusPlus::Document::Ptr &document);

private:
    CPlusPlus::Snapshot m_snapshot;
    QList<LanguageUtils::FakeMetaObject::ConstPtr> m_exportedTypes;
    QHash<QString, QString> m_contextProperties;
};

} // namespace QmlJS
