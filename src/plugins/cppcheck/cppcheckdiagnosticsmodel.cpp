// Copyright (C) 2019 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcheckdiagnosticsmodel.h"

#include "cppchecktr.h"

#include <debugger/analyzer/diagnosticlocation.h>

#include <utils/algorithm.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/utilsicons.h>

using namespace Debugger;

namespace Cppcheck::Internal {

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
            return Utils::FileIconProvider::icon(Utils::FilePath::fromString(m_filePath));
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
            const auto location = DiagnosticLocation(m_diagnostic.fileName,
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
                .arg(m_diagnostic.fileName.toUserOutput())
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
    setHeader({Tr::tr("Diagnostic")});
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
    if (!Utils::insert(m_diagnostics, diagnostic))
        return;

    if (m_diagnostics.size() == 1)
        emit hasDataChanged(true);

    const QString filePath = diagnostic.fileName.toString();
    FilePathItem *&filePathItem = m_filePathToItem[filePath];
    if (!filePathItem) {
        filePathItem = new FilePathItem(filePath);
        rootItem()->appendChild(filePathItem);
    }

    filePathItem->appendChild(new DiagnosticItem(diagnostic));
}

} // Cppcheck::Internal
