// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"

#include <QByteArray>
#include <QFutureInterface>
#include <QList>
#include <QString>

namespace Core {
namespace Internal {

class FileSystemFilter : public ILocatorFilter
{
    Q_OBJECT

public:
    explicit FileSystemFilter();
    void prepareSearch(const QString &entry) override;
    QList<LocatorFilterEntry> matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                         const QString &entry) override;
    void accept(const LocatorFilterEntry &selection,
                QString *newText, int *selectionStart, int *selectionLength) const override;
    void restoreState(const QByteArray &state) override;
    bool openConfigDialog(QWidget *parent, bool &needsRefresh) override;

protected:
    void saveState(QJsonObject &object) const final;
    void restoreState(const QJsonObject &object) final;

private:
    static MatchLevel matchLevelFor(const QRegularExpressionMatch &match, const QString &matchText);

    static const bool kIncludeHiddenDefault = true;
    bool m_includeHidden = kIncludeHiddenDefault;
    bool m_currentIncludeHidden = kIncludeHiddenDefault;
    QString m_currentDocumentDirectory;
};

} // namespace Internal
} // namespace Core
