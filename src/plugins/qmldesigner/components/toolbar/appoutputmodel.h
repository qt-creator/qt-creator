// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

#include <projectexplorer/runcontrol.h>
#include <utils/outputformat.h>
#include <utils/utilsicons.h>

#include <QColor>

class AppOutputParentModel;

class AppOutputChildModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QAbstractListModel *parentModel READ parentModel WRITE setParentModel NOTIFY parentModelChanged)
    Q_PROPERTY(int row READ row WRITE setRow NOTIFY rowChanged)

signals:
    void rowChanged();
    void parentModelChanged();

public:
    enum {
        MessageRole = Qt::DisplayRole,
        ColorRole = Qt::UserRole,
    };

    AppOutputChildModel(QObject *parent = nullptr);

    int row() const;
    void setRow(int row);

    QAbstractListModel *parentModel() const;
    void setParentModel(QAbstractListModel *model);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = MessageRole) const override;

    void addMessage(int row, const QString &message, const QColor &color);

private:
    int m_row = 0;
    AppOutputParentModel *m_parentModel = nullptr;
};

class AppOutputParentModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    enum { RunRole = Qt::DisplayRole, ColorRole = Qt::UserRole };

    Q_PROPERTY(QColor historyColor READ historyColor)
    Q_PROPERTY(QColor messageColor READ messageColor)
    Q_PROPERTY(QColor errorColor READ errorColor)
    Q_PROPERTY(QColor debugColor READ debugColor)

signals:
    void modelChanged();
    void messageAdded(int row, const QString &message, const QColor &color);

public:
    struct Message
    {
        QString message;
        QColor color;
    };

    struct Run
    {
        std::string timestamp;
        std::vector<Message> messages;
    };

    AppOutputParentModel(QObject *parent = nullptr);

    Q_INVOKABLE void resetModel();

    QColor historyColor() const;
    QColor messageColor() const;
    QColor errorColor() const;
    QColor debugColor() const;
    QColor warningColor() const;

    int messageCount(int row) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVariant runData(int runIdx, int msgIdx, int role) const;
    QVariant data(const QModelIndex &index, int role = RunRole) const override;

    Run *run(int row);

private:
    void setupRunControls();
    void initializeRuns(const QString &message = {});
    QColor colorFromFormat(Utils::OutputFormat format) const;

    const QColor m_messageColor = Utils::creatorColor(Utils::Theme::Token_Notification_Success_Default);
    const QColor m_historyColor = Utils::creatorColor(Utils::Theme::Token_Text_Muted);
    const QColor m_errorColor = Utils::creatorColor(Utils::Theme::CodeModel_Error_TextMarkColor);
    const QColor m_debugColor = Utils::creatorColor(Utils::Theme::Token_Notification_Success_Muted);
    const QColor m_warningColor = Utils::creatorColor(Utils::Theme::CodeModel_Warning_TextMarkColor);

    std::vector<Run> m_runs = {};
};
