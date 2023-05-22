// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ifindsupport.h"

#include <utils/filesearch.h>

#include <QPointer>

namespace Core {
namespace Internal {

class CurrentDocumentFind : public QObject
{
    Q_OBJECT

public:
    CurrentDocumentFind();

    void resetIncrementalSearch();
    void clearHighlights();
    bool supportsReplace() const;
    bool supportsSelectAll() const;
    Utils::FindFlags supportedFindFlags() const;
    QString currentFindString() const;
    QString completedFindString() const;

    bool isEnabled() const;
    IFindSupport *candidate() const;
    void highlightAll(const QString &txt, Utils::FindFlags findFlags);
    IFindSupport::Result findIncremental(const QString &txt, Utils::FindFlags findFlags);
    IFindSupport::Result findStep(const QString &txt, Utils::FindFlags findFlags);
    void selectAll(const QString &txt, Utils::FindFlags findFlags);
    void replace(const QString &before, const QString &after, Utils::FindFlags findFlags);
    bool replaceStep(const QString &before, const QString &after, Utils::FindFlags findFlags);
    int replaceAll(const QString &before, const QString &after, Utils::FindFlags findFlags);
    void defineFindScope();
    void clearFindScope();
    void acceptCandidate();

    void removeConnections();
    bool setFocusToCurrentFindSupport();

    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    void changed();
    void candidateChanged();

private:
    void updateCandidateFindFilter(QWidget *old, QWidget *now);
    void clearFindSupport();
    void aggregationChanged();
    void candidateAggregationChanged();
    void removeFindSupportConnections();

    QPointer<IFindSupport> m_currentFind;
    QPointer<QWidget> m_currentWidget;
    QPointer<IFindSupport> m_candidateFind;
    QPointer<QWidget> m_candidateWidget;
};

} // namespace Internal
} // namespace Core
