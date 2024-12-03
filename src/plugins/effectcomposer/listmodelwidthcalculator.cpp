// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "listmodelwidthcalculator.h"

#include <QApplication>

ListModelWidthCalculator::ListModelWidthCalculator(QObject *parent)
    : QObject(parent)
    , m_fontMetrics(qApp->font())
{}

void ListModelWidthCalculator::setModel(QAbstractItemModel *model)
{
    if (m_model == model)
        return;

    clearConnections();

    m_model = model;

    if (m_model) {
        m_connections << connect(
            m_model,
            &QAbstractItemModel::rowsInserted,
            this,
            &ListModelWidthCalculator::onSourceItemsInserted);

        m_connections << connect(
            m_model,
            &QAbstractItemModel::rowsRemoved,
            this,
            &ListModelWidthCalculator::onSourceItemsRemoved);

        m_connections << connect(
            m_model,
            &QAbstractItemModel::dataChanged,
            this,
            &ListModelWidthCalculator::onSourceDataChanged);

        m_connections << connect(
            m_model, &QAbstractItemModel::modelReset, this, &ListModelWidthCalculator::reset);
    }

    emit modelChanged();

    if (!updateRole())
        reset();
}

void ListModelWidthCalculator::setTextRole(const QString &role)
{
    if (m_textRole == role)
        return;
    m_textRole = role;
    emit textRoleChanged(m_textRole);
    updateRole();
}

void ListModelWidthCalculator::setFont(const QFont &font)
{
    if (m_font == font)
        return;

    m_font = font;
    emit fontChanged();
    reset();
}

QAbstractItemModel *ListModelWidthCalculator::model() const
{
    return m_model.get();
}

int ListModelWidthCalculator::maxWidth() const
{
    return m_maxWidth;
}

void ListModelWidthCalculator::clearConnections()
{
    for (QMetaObject::Connection &connection : m_connections)
        disconnect(connection);

    m_connections.clear();
}

void ListModelWidthCalculator::reset()
{
    int maxW = 0;
    m_data.clear();
    if (m_model) {
        const int rows = m_model->rowCount();
        m_data.reserve(rows);

        for (int row = 0; row < rows; ++row) {
            const QModelIndex &modelIdx = m_model->index(row, 0);
            const QString &text = modelIdx.data(m_role).toString();
            const int textWidth = widthOfText(text);
            m_data.append(textWidth);
            if (textWidth > maxW)
                maxW = textWidth;
        }
    }
    setMaxWidth(maxW);
}

/*!
 * \internal
 * \brief ListModelWidthCalculator::updateRole
 * \return True if role is updated. It means that reset() is called.
 */
bool ListModelWidthCalculator::updateRole()
{
    int newRole = -1;
    if (m_model && !m_textRole.isEmpty())
        newRole = m_model->roleNames().key(m_textRole.toUtf8());

    if (m_role != newRole) {
        m_role = newRole;
        reset();
        return true;
    }
    return false;
}

void ListModelWidthCalculator::setMaxWidth(int maxWidth)
{
    if (m_maxWidth == maxWidth)
        return;

    m_maxWidth = maxWidth;
    emit maxWidthChanged(m_maxWidth);
}

int ListModelWidthCalculator::widthOfText(const QString &str) const
{
    const int nIdx = str.indexOf('\n');
    const QString &firstLineText = (nIdx > -1) ? str.left(nIdx) : str;
    return std::ceil(m_fontMetrics.boundingRect(firstLineText).width());
}

void ListModelWidthCalculator::onSourceItemsInserted(const QModelIndex &, int first, int last)
{
    if (!isValidRow(first)) {
        reset();
        return;
    }

    int maxW = maxWidth();
    m_data.reserve(m_data.size() + last - first + 1);
    const QList<int> &tail = m_data.mid(first, -1);
    m_data.remove(first, -1);
    for (int row = first; row <= last; ++row) {
        const QModelIndex &modelIdx = m_model->index(row, 0);
        const QString &text = modelIdx.data(m_role).toString();
        const int textWidth = widthOfText(text);
        m_data.append(textWidth);
        if (textWidth > maxW)
            maxW = textWidth;
    }
    m_data.append(tail);
    setMaxWidth(maxW);
}

void ListModelWidthCalculator::onSourceItemsRemoved(const QModelIndex &parent, int first, int last)
{
    if (!isValidRow(first) || !isValidRow(last)) {
        reset();
        return;
    }

    int maxW = maxWidth();
    bool resetRequired = false;

    for (int row = first; row <= last; ++row) {
        const int savedWidth = m_data.at(row);
        if (savedWidth == maxW) {
            resetRequired = true;
            break;
        }
    }

    if (resetRequired)
        reset();
    else
        m_data.remove(first, last - first + 1);
}

void ListModelWidthCalculator::onSourceDataChanged(
    const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    if (!roles.contains(m_role))
        return;

    const int first = topLeft.row();
    const int last = bottomRight.row();

    if (!isValidRow(first) || !isValidRow(last)) {
        reset();
        return;
    }

    int maxW = maxWidth();

    for (int row = first; row <= last; ++row) {
        const QModelIndex &modelIdx = m_model->index(row, 0);
        const QString &text = modelIdx.data(m_role).toString();
        const int textWidth = widthOfText(text);
        const int savedWidth = m_data.at(row);
        if (textWidth == savedWidth)
            continue;

        if (textWidth > maxW)
            maxW = textWidth;

        m_data[row] = textWidth;
    }

    setMaxWidth(maxW);
}

bool ListModelWidthCalculator::isValidRow(int row)
{
    return row > -1 && row < m_data.size();
}
