/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "searchmodel.h"
#include "scxmltagutils.h"

using namespace ScxmlEditor::PluginInterface;
using namespace ScxmlEditor::Common;

SearchModel::SearchModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant SearchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return section == 0 ? tr("Type") : tr("Name");

    return QVariant();
}

QVariant SearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_allTags.count())
        return QVariant();

    ScxmlTag *tag = m_allTags[index.row()];
    switch (role) {
    case FilterRole: {
        if (index.column() == 0)
            return tag->tagName(true);
        else {
            QStringList keys = tag->attributeNames();
            QStringList values = tag->attributeValues();
            QStringList val;
            for (int i = 0; i < keys.count(); ++i)
                val << QString::fromLatin1("%1=%2").arg(keys[i]).arg(values[i]);
            return val.join(";");
        }
    }
    case Qt::DisplayRole: {
        if (index.column() == 0)
            return tag->tagName(true);
        else {
            QStringList keys = tag->attributeNames();
            QStringList values = tag->attributeValues();
            QStringList val;
            for (int i = 0; i < values.count(); ++i) {
                if (keys[i].contains(m_strFilter, Qt::CaseInsensitive) || values[i].contains(m_strFilter, Qt::CaseInsensitive))
                    val << QString::fromLatin1("%1=%2").arg(keys[i]).arg(values[i]);
            }
            return val.join(";");
        }
    }
    default:
        break;
    }

    return QVariant();
}

void SearchModel::setDocument(ScxmlDocument *document)
{
    if (m_document)
        m_document->disconnect(this);

    m_document = document;
    resetModel();

    if (m_document)
        connect(m_document, &ScxmlDocument::endTagChange, this, &SearchModel::tagChange);
}

void SearchModel::resetModel()
{
    beginResetModel();
    m_allTags.clear();
    if (m_document && m_document->rootTag()) {
        m_allTags << m_document->rootTag();
        TagUtils::findAllChildren(m_document->rootTag(), m_allTags);
    }
    endResetModel();
    emit layoutChanged();
}

void SearchModel::tagChange(ScxmlDocument::TagChange change, ScxmlTag *tag, const QVariant &value)
{
    Q_UNUSED(tag)
    Q_UNUSED(value)

    switch (change) {
    case ScxmlDocument::TagAddChild:
    case ScxmlDocument::TagRemoveChild:
    case ScxmlDocument::TagChangeParentAddChild:
    case ScxmlDocument::TagChangeParentRemoveChild:
        resetModel();
        break;
    default:
        break;
    }
}

int SearchModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}
int SearchModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_allTags.count();
}

ScxmlTag *SearchModel::tag(const QModelIndex &ind)
{
    if (ind.row() >= 0 && ind.row() < m_allTags.count())
        return m_allTags[ind.row()];

    return nullptr;
}

void SearchModel::setFilter(const QString &filter)
{
    m_strFilter = filter;
}
