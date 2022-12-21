// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/locator/ilocatorfilter.h>

#include <QString>
#include <QList>
#include <QFutureInterface>

namespace Core { class IEditor; }

namespace TextEditor {
namespace Internal {

class LineNumberFilter : public Core::ILocatorFilter
{
    Q_OBJECT

public:
    explicit LineNumberFilter(QObject *parent = nullptr);

    void prepareSearch(const QString &entry) override;
    QList<Core::LocatorFilterEntry> matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future,
                                               const QString &entry) override;
    void accept(const Core::LocatorFilterEntry &selection,
                QString *newText, int *selectionStart, int *selectionLength) const override;

private:
    bool m_hasCurrentEditor = false;
};

} // namespace Internal
} // namespace TextEditor
