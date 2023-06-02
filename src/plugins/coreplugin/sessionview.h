// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "sessionmodel.h"

#include <utils/itemviews.h>

#include <QAbstractTableModel>

namespace Core::Internal {

class SessionView : public Utils::TreeView
{
    Q_OBJECT

public:
    explicit SessionView(QWidget *parent = nullptr);

    void createNewSession();
    void deleteSelectedSessions();
    void cloneCurrentSession();
    void renameCurrentSession();
    void switchToCurrentSession();

    QString currentSession();
    SessionModel* sessionModel();
    void selectActiveSession();
    void selectSession(const QString &sessionName);

signals:
    void sessionActivated(const QString &session);
    void sessionsSelected(const QStringList &sessions);
    void sessionSwitched();

private:
    void showEvent(QShowEvent* event) override;
    void keyPressEvent(QKeyEvent *event) override;

    void deleteSessions(const QStringList &sessions);
    QStringList selectedSessions() const;

    SessionModel m_sessionModel;
};

} // namespace Core::Internal
