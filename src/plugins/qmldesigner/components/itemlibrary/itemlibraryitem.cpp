// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibraryitem.h"
#include "itemlibrarytracing.h"

#include <QApplication>

namespace QmlDesigner {

using ItemLibraryTracing::category;

ItemLibraryItem::ItemLibraryItem(const ItemLibraryEntry &itemLibraryEntry, bool isUsable, QObject *parent)
    : QObject(parent)
    , m_itemLibraryEntry(itemLibraryEntry)
    , m_isUsable(isUsable)
{
    NanotraceHR::Tracer tracer{"item library item constructor", category()};
}

ItemLibraryItem::~ItemLibraryItem()
{
    NanotraceHR::Tracer tracer{"item library item destructor", category()};
}

QString ItemLibraryItem::itemName() const
{
    NanotraceHR::Tracer tracer{"item library item name", category()};

    return QApplication::translate("itemlibrary", m_itemLibraryEntry.name().toUtf8());
}

QString ItemLibraryItem::typeName() const
{
    NanotraceHR::Tracer tracer{"item library item type name", category()};

    return QString::fromUtf8(m_itemLibraryEntry.typeName());
}

QString ItemLibraryItem::itemLibraryIconPath() const
{
    NanotraceHR::Tracer tracer{"item library item icon path", category()};

    if (m_itemLibraryEntry.customComponentSource().isEmpty()
            || !m_itemLibraryEntry.libraryEntryIconPath().isEmpty()) {
        return QStringLiteral("image://qmldesigner_itemlibrary/")
               + m_itemLibraryEntry.libraryEntryIconPath();
    } else {
        return QStringLiteral("image://itemlibrary_preview/")
               + m_itemLibraryEntry.customComponentSource();
    }
}

QString ItemLibraryItem::componentPath() const
{
    NanotraceHR::Tracer tracer{"item library item component path", category()};

    return m_itemLibraryEntry.customComponentSource();
}

QString ItemLibraryItem::requiredImport() const
{
    NanotraceHR::Tracer tracer{"item library item required import", category()};

    return m_itemLibraryEntry.requiredImport();
}

QString ItemLibraryItem::componentSource() const
{
    NanotraceHR::Tracer tracer{"item library item component source", category()};

    return m_itemLibraryEntry.customComponentSource();
}

QString ItemLibraryItem::toolTip() const
{
    NanotraceHR::Tracer tracer{"item library item tool tip", category()};

    return m_itemLibraryEntry.toolTip();
}

bool ItemLibraryItem::setVisible(bool isVisible)
{
    NanotraceHR::Tracer tracer{"item library item set visible", category()};

    if (isVisible != m_isVisible) {
        m_isVisible = isVisible;
        emit visibilityChanged();
        return true;
    }

    return false;
}

bool ItemLibraryItem::isVisible() const
{
    NanotraceHR::Tracer tracer{"item library item is visible", category()};

    return m_isVisible;
}

bool ItemLibraryItem::isUsable() const
{
    NanotraceHR::Tracer tracer{"item library item is usable", category()};

    return m_isUsable;
}

QVariant ItemLibraryItem::itemLibraryEntry() const
{
    NanotraceHR::Tracer tracer{"item library item library entry", category()};

    return QVariant::fromValue(m_itemLibraryEntry);
}
} // namespace QmlDesigner
