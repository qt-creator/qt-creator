/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#include "memcheckerrorview.h"

#include "suppressiondialog.h"
#include "valgrindsettings.h"

#include "xmlprotocol/error.h"
#include "xmlprotocol/errorlistmodel.h"
#include "xmlprotocol/frame.h"
#include "xmlprotocol/stack.h"
#include "xmlprotocol/modelhelpers.h"
#include "xmlprotocol/suppression.h"

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <utils/qtcassert.h>
#include <utils/icon.h>
#include <utils/theme/theme.h>

#include <QAction>

using namespace Valgrind::XmlProtocol;

namespace Valgrind {
namespace Internal {

MemcheckErrorView::MemcheckErrorView(QWidget *parent)
    : Debugger::DetailedErrorView(parent),
      m_settings(0)
{
    m_suppressAction = new QAction(this);
    m_suppressAction->setText(tr("Suppress Error"));
    const QIcon icon = Utils::Icon({
            {QLatin1String(":/utils/images/eye_open.png"), Utils::Theme::TextColorNormal},
            {QLatin1String(":/valgrind/images/suppressoverlay.png"), Utils::Theme::IconsErrorColor}},
            Utils::Icon::Tint | Utils::Icon::PunchEdges).icon();
    m_suppressAction->setIcon(icon);
    m_suppressAction->setShortcut(QKeySequence(Qt::Key_Delete));
    m_suppressAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_suppressAction, &QAction::triggered, this, &MemcheckErrorView::suppressError);
    addAction(m_suppressAction);
}

MemcheckErrorView::~MemcheckErrorView()
{
}

void MemcheckErrorView::setDefaultSuppressionFile(const QString &suppFile)
{
    m_defaultSuppFile = suppFile;
}

QString MemcheckErrorView::defaultSuppressionFile() const
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
    foreach (const QModelIndex &index, indizes) {
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
