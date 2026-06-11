// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ifindsupport.h"

#include <QPointer>

namespace Core::Internal {

class CurrentDocumentFind : public DelegatingFindSupport
{
    Q_OBJECT

public:
    CurrentDocumentFind();

    bool isEnabled() const;
    IFindSupport *candidate() const;
    void acceptCandidate();

    void removeConnections();
    bool setFocusToCurrentFindSupport();

    bool eventFilter(QObject *obj, QEvent *event) override;

    void selectAll(const QString &txt, Utils::FindFlags findFlags) override;
    int replaceAll(const QString &before, const QString &after, Utils::FindFlags findFlags) override;

signals:
    void candidateChanged();

private:
    void updateCandidateFindFilter();
    void clearFindSupport();
    void aggregationChanged();
    void candidateAggregationChanged();
    void removeFindSupportConnections();

    QPointer<IFindSupport> m_currentFind;
    QPointer<QWidget> m_currentWidget;
    QPointer<IFindSupport> m_candidateFind;
    QPointer<QWidget> m_candidateWidget;
};

} // namespace Core::Internal
