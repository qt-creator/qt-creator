// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"
#include "cpplocatordata.h"
#include "searchsymbols.h"

#include <coreplugin/locator/ilocatorfilter.h>

namespace CppEditor {

class CPPEDITOR_EXPORT CppLocatorFilter : public Core::ILocatorFilter
{
    Q_OBJECT

public:
    explicit CppLocatorFilter(CppLocatorData *locatorData);
    ~CppLocatorFilter() override;

    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
    void accept(const Core::LocatorFilterEntry &selection,
                QString *newText, int *selectionStart, int *selectionLength) const override;

protected:
    virtual IndexItem::ItemType matchTypes() const { return IndexItem::All; }
    virtual Core::LocatorFilterEntry filterEntryFromIndexItem(IndexItem::Ptr info);

protected:
    CppLocatorData *m_data = nullptr;
};

class CPPEDITOR_EXPORT CppClassesFilter : public CppLocatorFilter
{
    Q_OBJECT

public:
    explicit CppClassesFilter(CppLocatorData *locatorData);
    ~CppClassesFilter() override;

protected:
    IndexItem::ItemType matchTypes() const override { return IndexItem::Class; }
    Core::LocatorFilterEntry filterEntryFromIndexItem(IndexItem::Ptr info) override;
};

class CPPEDITOR_EXPORT CppFunctionsFilter : public CppLocatorFilter
{
    Q_OBJECT

public:
    explicit CppFunctionsFilter(CppLocatorData *locatorData);
    ~CppFunctionsFilter() override;

protected:
    IndexItem::ItemType matchTypes() const override { return IndexItem::Function; }
    Core::LocatorFilterEntry filterEntryFromIndexItem(IndexItem::Ptr info) override;
};

} // namespace CppEditor
