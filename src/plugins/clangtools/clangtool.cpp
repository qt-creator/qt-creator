/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "clangtool.h"

#include "clangselectablefilesdialog.h"
#include "clangtoolsconstants.h"
#include "clangtoolsdiagnostic.h"
#include "clangtoolsdiagnosticmodel.h"
#include "clangtoolsutils.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <cpptools/cppmodelmanager.h>

#include <debugger/analyzer/analyzermanager.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>

#include <utils/algorithm.h>
#include <utils/fancymainwindow.h>
#include <utils/utilsicons.h>

#include <QAction>
#include <QLabel>
#include <QSortFilterProxyModel>
#include <QToolButton>

using namespace Core;
using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace ClangTools {
namespace Internal {

static FileInfos sortedFileInfos(const QVector<CppTools::ProjectPart::Ptr> &projectParts)
{
    FileInfos fileInfos;

    for (CppTools::ProjectPart::Ptr projectPart : projectParts) {
        QTC_ASSERT(projectPart, continue);
        if (!projectPart->selectedForBuilding)
            continue;

        for (const CppTools::ProjectFile &file : projectPart->files) {
            QTC_ASSERT(file.kind != CppTools::ProjectFile::Unclassified, continue);
            QTC_ASSERT(file.kind != CppTools::ProjectFile::Unsupported, continue);
            if (file.path == CppTools::CppModelManager::configurationFileName())
                continue;

            if (file.active && CppTools::ProjectFile::isSource(file.kind)) {
                fileInfos.emplace_back(Utils::FileName::fromString(file.path),
                                       file.kind,
                                       projectPart);
            }
        }
    }

    Utils::sort(fileInfos, &FileInfo::file);
    fileInfos.erase(std::unique(fileInfos.begin(), fileInfos.end()), fileInfos.end());

    return fileInfos;
}

ClangTool::ClangTool(const QString &name)
    : m_name(name)
{
    m_diagnosticModel = new ClangToolsDiagnosticModel(this);

    m_startAction = Debugger::createStartAction();
    m_stopAction = Debugger::createStopAction();
}

ClangTool::~ClangTool()
{
    delete m_diagnosticView;
}

FileInfos ClangTool::collectFileInfos(Project *project, bool askUserForFileSelection) const
{
    auto projectInfo = CppTools::CppModelManager::instance()->projectInfo(project);
    QTC_ASSERT(projectInfo.isValid(), return FileInfos());

    const FileInfos allFileInfos = sortedFileInfos(projectInfo.projectParts());

    if (askUserForFileSelection) {
        SelectableFilesDialog dialog(projectInfo, allFileInfos);
        if (dialog.exec() == QDialog::Rejected)
            return FileInfos();
        return dialog.filteredFileInfos();
    } else {
        return allFileInfos;
    }
}

const QString &ClangTool::name() const
{
    return m_name;
}

void ClangTool::initDiagnosticView()
{
    m_diagnosticView->setFrameStyle(QFrame::NoFrame);
    m_diagnosticView->setAttribute(Qt::WA_MacShowFocusRect, false);
    m_diagnosticView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_diagnosticView->setAutoScroll(false);
}

QSet<Diagnostic> ClangTool::diagnostics() const
{
    return Utils::filtered(m_diagnosticModel->diagnostics(), [](const Diagnostic &diagnostic) {
        using CppTools::ProjectFile;
        return ProjectFile::isSource(ProjectFile::classify(diagnostic.location.filePath));
    });
}

void ClangTool::onNewDiagnosticsAvailable(const QList<Diagnostic> &diagnostics)
{
    QTC_ASSERT(m_diagnosticModel, return);
    m_diagnosticModel->addDiagnostics(diagnostics);
}

void ClangTool::setToolBusy(bool busy)
{
    QTC_ASSERT(m_diagnosticView, return);
    QCursor cursor(busy ? Qt::BusyCursor : Qt::ArrowCursor);
    m_diagnosticView->setCursor(cursor);
    m_toolBusy = busy;
}

} // namespace Internal
} // namespace ClangTools
