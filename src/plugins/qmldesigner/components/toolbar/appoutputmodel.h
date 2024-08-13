// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

#include <utils/outputformat.h>

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

    Q_PROPERTY(QColor historyColor READ historyColor WRITE setHistoryColor NOTIFY colorChanged)
    Q_PROPERTY(QColor messageColor READ messageColor WRITE setMessageColor NOTIFY colorChanged)
    Q_PROPERTY(QColor errorColor READ errorColor WRITE setErrorColor NOTIFY colorChanged)
    Q_PROPERTY(QColor debugColor READ debugColor WRITE setDebugColor NOTIFY colorChanged)

signals:
    void colorChanged();
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
    void setHistoryColor(const QColor &color);

    QColor messageColor() const;
    void setMessageColor(const QColor &color);

    QColor errorColor() const;
    void setErrorColor(const QColor &color);

    QColor debugColor() const;
    void setDebugColor(const QColor &color);

    int messageCount(int row) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVariant runData(int runIdx, int msgIdx, int role) const;
    QVariant data(const QModelIndex &index, int role = RunRole) const override;

    Run *run(int row);

private:
    void setupRunControls();
    QColor colorFromFormat(Utils::OutputFormat format) const;

    QColor m_historyColor = Qt::gray;
    QColor m_messageColor = Qt::green;
    QColor m_errorColor = Qt::red;
    QColor m_debugColor = Qt::magenta;

#if 1
    std::vector<Run> m_runs = {};
#else
    std::vector<Run> m_runs = {
        {"04.04.2024 10:30am", {{"line 1", "grey"}, {"line 2", "grey"}, {"line 3", "grey"}}},
        {"04.04.2024 11:30am", {{"line 4", "grey"}, {"line 5", "grey"}, {"line 6", "grey"}}},
        {"04.04.2024 12:30am", {{"line 7", "grey"}, {"line 8", "grey"}, {"line 9", "grey"}}},
        {"04.04.2024 13:40am",
         {{"line 7", "grey"},
          {"line 8", "grey"},
          {"line 9", "grey"},
          {"line 7", "grey"},
          {"line 8", "grey"},
          {"line 9", "grey"},
          {"line 7", "grey"},
          {"line 8", "grey"},
          {"line 9", "grey"},
          {"line 7", "grey"},
          {"line 8", "grey"},
          {"line 9", "grey"}}},
        {"04.04.2024 14:30am", {{"line 7", "grey"}, {"line 8", "grey"}, {"line 9", "grey"}}},
        {"04.04.2024 15:30am", {{"line 7", "grey"}, {"line 8", "grey"}, {"line 9", "grey"}}},
        {"04.04.2024 16:30am", {{"line 7", "grey"}, {"line 8", "grey"}, {"line 9", "grey"}}},
        {"04.04.2024 17:30am", {{"line 7", "grey"}, {"line 8", "grey"}, {"line 9", "grey"}}},
        {"04.04.2024 18:30am", {{"line 7", "grey"}, {"line 8", "grey"}, {"line 9", "grey"}}},
        {"04.04.2024 19:30am", {{"line 7", "grey"}, {"line 8", "grey"}, {"line 9", "grey"}}},
        {"04.04.2024 20:30am", {{"line 7", "grey"}, {"line 8", "grey"}, {"line 9", "grey"}}},
        {"04.04.2024 21:30am", {{"line 7", "grey"}, {"line 8", "grey"}, {"line 9", "grey"}}},
        {"04.04.2024 22:30am", {{"line 7", "grey"}, {"line 8", "grey"}, {"line 9", "grey"}}},
    };
#endif
};
