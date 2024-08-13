// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "appoutputmodel.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/runcontrol.h>

AppOutputChildModel::AppOutputChildModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int AppOutputChildModel::row() const
{
    return m_row;
}

void AppOutputChildModel::setRow(int row)
{
    m_row = row;
}

QAbstractListModel *AppOutputChildModel::parentModel() const
{
    return m_parentModel;
}

void AppOutputChildModel::setParentModel(QAbstractListModel *model)
{
    if (AppOutputParentModel *sm = qobject_cast<AppOutputParentModel *>(model)) {
        if (m_parentModel != sm) {
            m_parentModel = sm;
            connect(m_parentModel,
                    &AppOutputParentModel::messageAdded,
                    this,
                    &AppOutputChildModel::addMessage);
            emit parentModelChanged();
        }
    }
}

void AppOutputChildModel::addMessage(int row, const QString &message, const QColor &color)
{
    if (row != m_row)
        return;

    if (!m_parentModel)
        return;

    if (AppOutputParentModel::Run *run = m_parentModel->run(m_row)) {
        int at = run->messages.size();
        beginInsertRows(QModelIndex(), at, at);
        run->messages.push_back({message, color});
        endInsertRows();
    }
}

int AppOutputChildModel::rowCount(const QModelIndex &) const
{
    if (m_parentModel)
        return m_parentModel->messageCount(m_row);
    return 0;
}

QHash<int, QByteArray> AppOutputChildModel::roleNames() const
{
    QHash<int, QByteArray> out;
    out[MessageRole] = "message";
    out[ColorRole] = "messageColor";
    return out;
}

QVariant AppOutputChildModel::data(const QModelIndex &index, int role) const
{
    if (m_parentModel)
        return m_parentModel->runData(m_row, index.row(), role);

    return {};
}


AppOutputParentModel::Run *AppOutputParentModel::run(int row)
{
    if (row < static_cast<int>(m_runs.size()) && row >= 0)
        return &m_runs.at(row);

    return nullptr;
}

AppOutputParentModel::AppOutputParentModel(QObject *parent)
    : QAbstractListModel(parent)
{
    setupRunControls();
}

QColor AppOutputParentModel::historyColor() const
{
    return m_historyColor;
}

void AppOutputParentModel::setHistoryColor(const QColor &color)
{
    if (m_historyColor != color) {
        m_historyColor = color;
        emit colorChanged();
    }
}

QColor AppOutputParentModel::messageColor() const
{
    return m_messageColor;
}

void AppOutputParentModel::setMessageColor(const QColor &color)
{
    if (m_messageColor != color) {
        m_messageColor = color;
        emit colorChanged();
    }
}

QColor AppOutputParentModel::errorColor() const
{
    return m_errorColor;
}

void AppOutputParentModel::setErrorColor(const QColor &color)
{
    if (m_errorColor != color) {
        m_errorColor = color;
        emit colorChanged();
    }
}

QColor AppOutputParentModel::debugColor() const
{
    return m_debugColor;
}

void AppOutputParentModel::setDebugColor(const QColor &color)
{
    if (m_debugColor != color) {
        m_debugColor = color;
        emit colorChanged();
    }
}

void AppOutputParentModel::resetModel()
{
    beginResetModel();
    m_runs.clear();
    endResetModel();
    emit modelChanged();
}

int AppOutputParentModel::messageCount(int row) const
{
    if (row < static_cast<int>(m_runs.size()) && row >= 0)
        return m_runs.at(row).messages.size();

    return 0;
}

int AppOutputParentModel::rowCount(const QModelIndex &) const
{
    return m_runs.size();
}

QHash<int, QByteArray> AppOutputParentModel::roleNames() const
{
    QHash<int, QByteArray> out;
    out[RunRole] = "run";
    out[ColorRole] = "blockColor";
    return out;
}

QVariant AppOutputParentModel::runData(int runIdx, int msgIdx, int role) const
{
    if (runIdx < static_cast<int>(m_runs.size()) && runIdx >= 0) {
        if (msgIdx < static_cast<int>(m_runs.at(runIdx).messages.size()) && msgIdx >= 0) {
            if (role == AppOutputChildModel::MessageRole)
                return m_runs.at(runIdx).messages.at(msgIdx).message;
            else if (role == AppOutputChildModel::ColorRole) {
                if (runIdx == static_cast<int>(m_runs.size()) - 1)
                    return m_runs.at(runIdx).messages.at(msgIdx).color;
                else
                    return m_historyColor;
            }
        }
    }
    return {};
}

QVariant AppOutputParentModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() < rowCount()) {
        int row = index.row();
        if (role == RunRole) {
            return m_runs.at(row).timestamp.c_str();
        } else if (role == ColorRole) {
            int last = static_cast<int>(m_runs.size()) - 1;
            return row < last ? m_historyColor : m_messageColor;
        } else {
            qWarning() << Q_FUNC_INFO << "invalid role";
        }
    } else {
        qWarning() << Q_FUNC_INFO << "invalid index";
    }
    return {};
}

void AppOutputParentModel::setupRunControls()
{
    auto *explorerPlugin = ProjectExplorer::ProjectExplorerPlugin::instance();
    connect(explorerPlugin,
            &ProjectExplorer::ProjectExplorerPlugin::runControlStarted,
            [this](ProjectExplorer::RunControl *rc) {
                AppOutputParentModel::Run run;
                run.timestamp = QTime::currentTime().toString().toStdString();
                run.messages.push_back({rc->commandLine().displayName(), m_messageColor});

                beginResetModel();
                m_runs.push_back(run);
                endResetModel();

                connect(rc,
                        &ProjectExplorer::RunControl::appendMessage,
                        [this](const QString &out, Utils::OutputFormat format) {
                            if (!m_runs.empty()) {
                                int row = static_cast<int>(m_runs.size()) - 1;
                                emit messageAdded(row, out.trimmed(), colorFromFormat(format));
                            }
                        });
            });
}

QColor AppOutputParentModel::colorFromFormat(Utils::OutputFormat format) const
{
    switch (format) {
    case Utils::NormalMessageFormat:
    case Utils::LogMessageFormat:
    case Utils::StdOutFormat:
    case Utils::GeneralMessageFormat:
        return m_messageColor;
    case Utils::DebugFormat:
        return m_debugColor;
    case Utils::StdErrFormat:
        return m_errorColor;
    default:
        return m_messageColor;
    }
}
