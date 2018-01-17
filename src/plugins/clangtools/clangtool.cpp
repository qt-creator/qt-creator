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

#include "clangtoolsconstants.h"
#include "clangtoolsdiagnostic.h"
#include "clangtoolsdiagnosticmodel.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <debugger/analyzer/analyzermanager.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/target.h>
#include <projectexplorer/session.h>

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

ClangTool::ClangTool(const QString &name)
    : m_name(name)
{
    m_diagnosticModel = new ClangToolsDiagnosticModel(this);

    m_startAction = Debugger::createStartAction();
    m_stopAction = Debugger::createStopAction();
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

QList<Diagnostic> ClangTool::diagnostics() const
{
    return m_diagnosticModel->diagnostics();
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
