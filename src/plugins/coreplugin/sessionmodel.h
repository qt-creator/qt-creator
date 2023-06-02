// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <QAbstractTableModel>

#include <functional>

namespace Core {

const char SESSION_BASE_ID[] = "Welcome.OpenSession";

namespace Internal { class SessionNameInputDialog; }

class CORE_EXPORT SessionModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum {
        DefaultSessionRole = Qt::UserRole+1,
        LastSessionRole,
        ActiveSessionRole,
        ShortcutRole
    };

    explicit SessionModel(QObject *parent = nullptr);

    int indexOfSession(const QString &session);
    QString sessionAt(int row) const;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    Q_SCRIPTABLE bool isDefaultVirgin() const;

signals:
    void sessionSwitched();
    void sessionCreated(const QString &sessionName);

public slots:
    void resetSessions();
    void newSession(QWidget *parent);
    void cloneSession(QWidget *parent, const QString &session);
    void deleteSessions(const QStringList &sessions);
    void renameSession(QWidget *parent, const QString &session);
    void switchToSession(const QString &session);

private:
    void runSessionNameInputDialog(Internal::SessionNameInputDialog *sessionInputDialog,
                                   std::function<void(const QString &)> createSession);

    QStringList m_sortedSessions;
    int m_currentSortColumn = 0;
    Qt::SortOrder m_currentSortOrder = Qt::AscendingOrder;
};

} // namespace Core
