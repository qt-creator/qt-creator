/****************************************************************************
**
** Copyright (C) 2019 Sergey Morozov
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

#include "cppcheckdiagnosticsmodel.h"

#include <coreplugin/fileiconprovider.h>

#include <debugger/analyzer/diagnosticlocation.h>

#include <utils/utilsicons.h>

namespace Cppcheck {
namespace Internal {

using namespace Debugger;

FilePathItem::FilePathItem(const QString &filePath)
    : m_filePath(filePath)
{}

QVariant FilePathItem::data(int column, int role) const
{
    if (column == DiagnosticsModel::DiagnosticColumn) {
        switch (role) {
        case Qt::DisplayRole:
            return m_filePath;
        case Qt::DecorationRole:
            return Core::FileIconProvider::icon(m_filePath);
        case Debugger::DetailedErrorView::FullTextRole:
            return m_filePath;
        default:
            return QVariant();
        }
    }

    return QVariant();
}

DiagnosticItem::DiagnosticItem(const Diagnostic &diagnostic)
    : m_diagnostic(diagnostic)
{}

static QIcon getIcon(const Diagnostic::Severity severity)
{
    switch (severity) {
    case Diagnostic::Severity::Error:
        return Utils::Icons::CRITICAL.icon();
    case Diagnostic::Severity::Warning:
        return Utils::Icons::WARNING.icon();
    default:
        return Utils::Icons::INFO.icon();
    }
}

QVariant DiagnosticItem::data(int column, int role) const
{
    if (column == DiagnosticsModel::DiagnosticColumn) {
        switch (role) {
        case DetailedErrorView::LocationRole: {
            const auto location = DiagnosticLocation(m_diagnostic.fileName.toString(),
                                                     m_diagnostic.lineNumber,
                                                     0);
            return QVariant::fromValue(location);
        }
        case Qt::DisplayRole:
            return QString("%1: %2").arg(m_diagnostic.lineNumber).arg(m_diagnostic.message);
        case Qt::ToolTipRole:
            return QString("%1: %2").arg(m_diagnostic.severityText, m_diagnostic.checkId);
        case Qt::DecorationRole:
            return getIcon(m_diagnostic.severity);
        case Debugger::DetailedErrorView::FullTextRole:
            return QString("%1:%2: %3")
                .arg(m_diagnostic.fileName.toString())
                .arg(m_diagnostic.lineNumber)
                .arg(m_diagnostic.message);
        default:
            return QVariant();
        }
    }

    return QVariant();
}

DiagnosticsModel::DiagnosticsModel(QObject *parent)
    : BaseModel(parent)
{
    setHeader({tr("Diagnostic")});
}

void DiagnosticsModel::clear()
{
    const auto hasData = !m_diagnostics.isEmpty();
    m_filePathToItem.clear();
    m_diagnostics.clear();
    BaseModel::clear();
    if (hasData)
        emit hasDataChanged(false);
}

void DiagnosticsModel::add(const Diagnostic &diagnostic)
{
    if (m_diagnostics.contains(diagnostic))
        return;

    const auto hasData = !m_diagnostics.isEmpty();
    m_diagnostics.insert(diagnostic);
    if (!hasData)
        emit hasDataChanged(true);

    const QString filePath = diagnostic.fileName.toString();
    FilePathItem *&filePathItem = m_filePathToItem[filePath];
    if (!filePathItem) {
        filePathItem = new FilePathItem(filePath);
        rootItem()->appendChild(filePathItem);
    }

    filePathItem->appendChild(new DiagnosticItem(diagnostic));
}

} // namespace Internal
} // namespace Cppcheck
