// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scxmleditortr.h"
#include "scxmltagutils.h"
#include "searchmodel.h"

using namespace ScxmlEditor::PluginInterface;
using namespace ScxmlEditor::Common;

SearchModel::SearchModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

QVariant SearchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return section == 0 ? Tr::tr("Type") : Tr::tr("Name");

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
