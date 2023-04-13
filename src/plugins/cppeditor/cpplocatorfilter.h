// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "indexitem.h"

#include <coreplugin/locator/ilocatorfilter.h>

namespace CppEditor {

Core::LocatorMatcherTasks CPPEDITOR_EXPORT cppMatchers(Core::MatcherType type);

class CPPEDITOR_EXPORT CppLocatorFilter : public Core::ILocatorFilter
{
    Q_OBJECT

public:
    explicit CppLocatorFilter();

    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
protected:
    virtual IndexItem::ItemType matchTypes() const { return IndexItem::All; }
    virtual Core::LocatorFilterEntry filterEntryFromIndexItem(IndexItem::Ptr info);
};

class CPPEDITOR_EXPORT CppClassesFilter : public CppLocatorFilter
{
    Q_OBJECT

public:
    explicit CppClassesFilter();

protected:
    IndexItem::ItemType matchTypes() const override { return IndexItem::Class; }
    Core::LocatorFilterEntry filterEntryFromIndexItem(IndexItem::Ptr info) override;
};

class CPPEDITOR_EXPORT CppFunctionsFilter : public CppLocatorFilter
{
    Q_OBJECT

public:
    explicit CppFunctionsFilter();

protected:
    IndexItem::ItemType matchTypes() const override { return IndexItem::Function; }
    Core::LocatorFilterEntry filterEntryFromIndexItem(IndexItem::Ptr info) override;
};

} // namespace CppEditor
