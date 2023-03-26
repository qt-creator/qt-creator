// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "memcheckerrorview.h"

#include "suppressiondialog.h"
#include "valgrindsettings.h"
#include "valgrindtr.h"

#include "xmlprotocol/error.h"
#include "xmlprotocol/errorlistmodel.h"
#include "xmlprotocol/suppression.h"

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>

#include <utils/qtcassert.h>
#include <utils/icon.h>
#include <utils/theme/theme.h>

#include <QAction>

using namespace Utils;
using namespace Valgrind::XmlProtocol;

namespace Valgrind {
namespace Internal {

MemcheckErrorView::MemcheckErrorView(QWidget *parent)
    : Debugger::DetailedErrorView(parent)
{
    m_suppressAction = new QAction(this);
    m_suppressAction->setText(Tr::tr("Suppress Error"));
    const QIcon icon = Icon({
            {":/utils/images/eye_open.png", Theme::TextColorNormal},
            {":/valgrind/images/suppressoverlay.png", Theme::IconsErrorColor}},
            Icon::Tint | Icon::PunchEdges).icon();
    m_suppressAction->setIcon(icon);
    m_suppressAction->setShortcuts({QKeySequence::Delete, QKeySequence::Backspace});
    m_suppressAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_suppressAction, &QAction::triggered, this, &MemcheckErrorView::suppressError);
    addAction(m_suppressAction);
}

MemcheckErrorView::~MemcheckErrorView() = default;

void MemcheckErrorView::setDefaultSuppressionFile(const FilePath &suppFile)
{
    m_defaultSuppFile = suppFile;
}

FilePath MemcheckErrorView::defaultSuppressionFile() const
{
    return m_defaultSuppFile;
}

// slot, can (for now) be invoked either when the settings were modified *or* when the active
// settings object has changed.
void MemcheckErrorView::settingsChanged(ValgrindBaseSettings *settings)
{
    QTC_ASSERT(settings, return);
    m_settings = settings;
}

void MemcheckErrorView::suppressError()
{
    SuppressionDialog::maybeShow(this);
}

QList<QAction *> MemcheckErrorView::customActions() const
{
    QList<QAction *> actions;
    const QModelIndexList indizes = selectionModel()->selectedRows();
    QTC_ASSERT(!indizes.isEmpty(), return actions);

    bool hasErrors = false;
    for (const QModelIndex &index : indizes) {
        Error error = model()->data(index, ErrorListModel::ErrorRole).value<Error>();
        if (!error.suppression().isNull()) {
            hasErrors = true;
            break;
        }
    }
    m_suppressAction->setEnabled(hasErrors);
    actions << m_suppressAction;
    return actions;
}

} // namespace Internal
} // namespace Valgrind
