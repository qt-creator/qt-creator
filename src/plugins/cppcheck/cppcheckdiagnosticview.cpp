// Copyright (C) 2019 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcheckdiagnosticview.h"

#include "cppchecktr.h"

#include <coreplugin/editormanager/editormanager.h>

#include <debugger/analyzer/diagnosticlocation.h>

using namespace Debugger;

namespace Cppcheck::Internal {

DiagnosticView::DiagnosticView(QWidget *parent)
    : DetailedErrorView(parent)
{
    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setAutoScroll(false);
    sortByColumn(DiagnosticColumn, Qt::AscendingOrder);
    setObjectName("CppcheckIssuesView");
    setWindowTitle(Tr::tr("Cppcheck Diagnostics"));
    setHeaderHidden(true);
}

void DiagnosticView::goNext()
{
    const auto totalFiles = model()->rowCount();
    if (totalFiles == 0)
        return;

    const QModelIndex currentIndex = selectionModel()->currentIndex();
    const QModelIndex parent = currentIndex.parent();
    const auto onDiagnostic = parent.isValid();
    if (onDiagnostic && currentIndex.row() < model()->rowCount(parent) - 1) {
        selectIndex(currentIndex.sibling(currentIndex.row() + 1, 0));
        return;
    }
    auto newFileRow = 0;
    if (!currentIndex.isValid()) // not selected
        newFileRow = 0;
    else if (!onDiagnostic) // selected file
        newFileRow = currentIndex.row();
    else // selected last item in file
        newFileRow = parent.row() == totalFiles - 1 ? 0 : parent.row() + 1;
    const QModelIndex newParent = model()->index(newFileRow, 0);
    selectIndex(model()->index(0, 0, newParent));
}

void DiagnosticView::goBack()
{
    const auto totalFiles = model()->rowCount();
    if (totalFiles == 0)
        return;

    const QModelIndex currentIndex = selectionModel()->currentIndex();
    const QModelIndex parent = currentIndex.parent();
    const auto onDiagnostic = parent.isValid();
    if (onDiagnostic && currentIndex.row() > 0) {
        selectIndex(currentIndex.sibling(currentIndex.row() - 1, 0));
        return;
    }
    auto newFileRow = 0;
    if (!currentIndex.isValid()) // not selected
        newFileRow = totalFiles - 1;
    else if (!onDiagnostic) // selected file
        newFileRow = currentIndex.row() == 0 ? totalFiles - 1 : currentIndex.row() - 1;
    else // selected first item in file
        newFileRow = parent.row() == 0 ? totalFiles - 1 : parent.row() - 1;
    const QModelIndex newParent = model()->index(newFileRow, 0);
    const auto newParentRows = model()->rowCount(newParent);
    selectIndex(model()->index(newParentRows - 1, 0, newParent));
}

DiagnosticView::~DiagnosticView() = default;

void DiagnosticView::openEditorForCurrentIndex()
{
    const QVariant v = model()->data(currentIndex(), Debugger::DetailedErrorView::LocationRole);
    const auto loc = v.value<Debugger::DiagnosticLocation>();
    if (loc.isValid())
        Core::EditorManager::openEditorAt(Utils::Link(loc.filePath, loc.line, loc.column - 1));
}

void DiagnosticView::mouseDoubleClickEvent(QMouseEvent *event)
{
    openEditorForCurrentIndex();
    DetailedErrorView::mouseDoubleClickEvent(event);
}

} // Cppcheck::Internal
