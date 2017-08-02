/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "cppparsecontext.h"

#include "cppeditorwidget.h"

#include <QAction>
#include <QDir>
#include <QDebug>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

namespace CppEditor {
namespace Internal {

void ParseContextModel::update(const CppTools::ProjectPartInfo &projectPartInfo)
{
    beginResetModel();
    reset(projectPartInfo);
    endResetModel();

    emit updated(areMultipleAvailable());
}

QString ParseContextModel::currentToolTip() const
{
    const QModelIndex index = createIndex(m_currentIndex, 0);
    if (!index.isValid())
        return QString();

    return tr("<p><b>Active Parse Context</b>:<br/>%1</p>"
              "<p>Multiple parse contexts (set of defines, include paths, and so on) "
              "are available for this file.</p>"
              "<p>Choose a parse context to set it as the preferred one. "
              "Clear the preference from the context menu.</p>")
                .arg(data(index, Qt::ToolTipRole).toString());
}

void ParseContextModel::setPreferred(int index)
{
    if (index < 0)
        return;

    emit preferredParseContextChanged(m_projectParts[index]->id());
}

void ParseContextModel::clearPreferred()
{
    emit preferredParseContextChanged(QString());
}

bool ParseContextModel::areMultipleAvailable() const
{
    return m_projectParts.size() >= 2;
}

void ParseContextModel::reset(const CppTools::ProjectPartInfo &projectPartInfo)
{
    // Sort
    m_hints = projectPartInfo.hints;
    m_projectParts = projectPartInfo.projectParts;
    Utils::sort(m_projectParts, &CppTools::ProjectPart::displayName);

    // Determine index for current
    const QString id = projectPartInfo.projectPart->id();
    m_currentIndex = Utils::indexOf(m_projectParts, [id](const CppTools::ProjectPart::Ptr &pp) {
        return pp->id() == id;
    });
    QTC_CHECK(m_currentIndex >= 0);
}

int ParseContextModel::currentIndex() const
{
    return m_currentIndex;
}

bool ParseContextModel::isCurrentPreferred() const
{
    return m_hints & CppTools::ProjectPartInfo::IsPreferredMatch;
}

QString ParseContextModel::currentId() const
{
    if (m_currentIndex < 0)
        return QString();

    return m_projectParts[m_currentIndex]->id();
}

int ParseContextModel::rowCount(const QModelIndex &) const
{
    return m_projectParts.size();
}

QVariant ParseContextModel::data(const QModelIndex &index, int role) const
{
    if (m_projectParts.isEmpty())
        return QVariant();

    const int row = index.row();
    if (role == Qt::DisplayRole)
        return m_projectParts[row]->displayName;
    else if (role == Qt::ToolTipRole)
        return QDir::toNativeSeparators(m_projectParts[row]->projectFile);

    return QVariant();
}

ParseContextWidget::ParseContextWidget(ParseContextModel &parseContextModel, QWidget *parent)
    : QComboBox(parent)
    , m_parseContextModel(parseContextModel)
{
    // Set up context menu with a clear action
    setContextMenuPolicy(Qt::ActionsContextMenu);
    m_clearPreferredAction = new QAction(tr("Clear Preferred Parse Context"), this);
    connect(m_clearPreferredAction,  &QAction::triggered,[&]() {
        m_parseContextModel.clearPreferred();
    });
    addAction(m_clearPreferredAction);

    // Set up sync of this widget and model in both directions
    connect(this,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::activated),
            &m_parseContextModel,
            &ParseContextModel::setPreferred);
    connect(&m_parseContextModel, &ParseContextModel::updated,
            this, &ParseContextWidget::syncToModel);

    // Set up model
    setModel(&m_parseContextModel);
}

void ParseContextWidget::syncToModel()
{
    const int index = m_parseContextModel.currentIndex();
    if (index < 0)
        return; // E.g. editor was duplicated but no project context was determined yet.

    if (currentIndex() != index)
        setCurrentIndex(index);

    setToolTip(m_parseContextModel.currentToolTip());

    const bool isPreferred = m_parseContextModel.isCurrentPreferred();
    m_clearPreferredAction->setEnabled(isPreferred);
    CppEditorWidget::updateWidgetHighlighting(this, isPreferred);
}

} // namespace Internal
} // namespace CppEditor
