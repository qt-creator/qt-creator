// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "collapsibleframe.h"

#include <acp/acp.h>

#include <utils/filepath.h>

#include <QHash>

class QPushButton;
class QVBoxLayout;

namespace AcpClient::Internal {

class SessionPickerWidget : public CollapsibleFrame
{
    Q_OBJECT

public:
    struct NewSessionTarget
    {
        QString label;
        Utils::FilePath cwd;
        QString tooltip;
    };

    explicit SessionPickerWidget(QWidget *parent = nullptr);

    void setCurrentProjectDir(const Utils::FilePath &dir);
    void setNewSessionTargets(const QList<NewSessionTarget> &targets);

    void setInitialSessions(const QList<Acp::SessionInfo> &sessions,
                            const std::optional<QString> &nextCursor);
    void appendSessions(const QList<Acp::SessionInfo> &sessions,
                        const std::optional<QString> &nextCursor);

    void setResolved(const QString &title);

signals:
    void sessionSelected(const QString &sessionId, const Utils::FilePath &cwd);
    void loadMoreRequested(const QString &cursor);
    void newSessionRequested(const Utils::FilePath &cwd);

private:
    struct Group
    {
        CollapsibleFrame *frame = nullptr;
        QVBoxLayout *layout = nullptr;
    };

    void addSessionItem(const Acp::SessionInfo &session);
    void requestNewSession(const Utils::FilePath &cwd);
    void requestCustomDirectorySession();
    void updateLoadMoreButton();
    void updateEmptyState();
    void clearGroups();
    void updateCollapseState();
    Group &ensureGroup(const QString &cwd);

    QVBoxLayout *m_newSessionContainer = nullptr;
    QVBoxLayout *m_currentGroupContainer = nullptr;
    QVBoxLayout *m_otherGroupsContainer = nullptr;
    QHash<QString, Group> m_groups;
    QString m_currentGroupKey;
    QFrame *m_topSeparator = nullptr;
    QFrame *m_middleSeparator = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QPushButton *m_loadMoreButton = nullptr;
    Utils::FilePath m_currentProjectDir;
    QString m_nextCursor;
    bool m_resolved = false;
};

} // namespace AcpClient::Internal
