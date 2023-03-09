// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"

#include <coreplugin/editormanager/documentmodel.h>

#include <QMutex>

namespace Core {
namespace Internal {

class OpenDocumentsFilter : public ILocatorFilter
{
    Q_OBJECT

public:
    OpenDocumentsFilter();
    QList<LocatorFilterEntry> matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                         const QString &entry) override;
public slots:
    void slotDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                         const QVector<int> &roles);
    void slotRowsInserted(const QModelIndex &, int first, int last);
    void slotRowsRemoved(const QModelIndex &, int first, int last);

private:
    class Entry
    {
    public:
        Utils::FilePath fileName;
        QString displayName;
    };

    QList<Entry> editors() const;
    void refreshInternally();

    mutable QMutex m_mutex;
    QList<Entry> m_editors;
};

} // namespace Internal
} // namespace Core
