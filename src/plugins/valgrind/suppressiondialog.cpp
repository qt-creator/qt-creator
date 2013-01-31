/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "suppressiondialog.h"

#include "memcheckerrorview.h"
#include "valgrindsettings.h"

#include "xmlprotocol/suppression.h"
#include "xmlprotocol/errorlistmodel.h"
#include "xmlprotocol/stack.h"
#include "xmlprotocol/frame.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <utils/pathchooser.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QFile>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>

using namespace Analyzer;
using namespace Valgrind::XmlProtocol;

namespace Valgrind {
namespace Internal {

static QString suppressionText(const Error &error)
{
    Suppression sup(error.suppression());

    // workaround: https://bugs.kde.org/show_bug.cgi?id=255822
    if (sup.frames().size() >= 24)
        sup.setFrames(sup.frames().mid(0, 23));
    QTC_ASSERT(sup.frames().size() < 24, /**/);

    // try to set some useful name automatically, instead of "insert_name_here"
    // we take the last stack frame and append the suppression kind, e.g.:
    // QDebug::operator<<(bool) [Memcheck:Cond]
    if (!error.stacks().isEmpty() && !error.stacks().first().frames().isEmpty()) {
        const Frame frame = error.stacks().first().frames().first();

        QString newName;
        if (!frame.functionName().isEmpty())
            newName = frame.functionName();
        else if (!frame.object().isEmpty())
            newName = frame.object();

        if (!newName.isEmpty())
            sup.setName(newName + QLatin1Char('[') + sup.kind() + QLatin1Char(']'));
    }

    return sup.toString();
}

/// @p error input error, which might get hidden when it has the same stack
/// @p suppressed the error that got suppressed already
static bool equalSuppression(const Error &error, const Error &suppressed)
{
    if (error.kind() != suppressed.kind() || error.suppression().isNull())
        return false;

    const SuppressionFrames errorFrames = error.suppression().frames();
    const SuppressionFrames suppressedFrames = suppressed.suppression().frames();

    // limit to 23 frames, see: https://bugs.kde.org/show_bug.cgi?id=255822
    if (qMin(23, suppressedFrames.size()) > errorFrames.size())
        return false;

    int frames = 23;
    if (errorFrames.size() < frames)
        frames = errorFrames.size();

    if (suppressedFrames.size() < frames)
        frames = suppressedFrames.size();

    for (int i = 0; i < frames; ++i)
        if (errorFrames.at(i) != suppressedFrames.at(i))
            return false;

    return true;
}

bool sortIndizesReverse(const QModelIndex &l, const QModelIndex &r)
{
    return l.row() > r.row();
}

SuppressionDialog::SuppressionDialog(MemcheckErrorView *view, const QList<Error> &errors)
  : m_view(view),
    m_settings(view->settings()),
    m_cleanupIfCanceled(false),
    m_errors(errors),
    m_fileChooser(new Utils::PathChooser(this)),
    m_suppressionEdit(new QPlainTextEdit(this))
{
    setWindowTitle(tr("Save Suppression"));

    QLabel *fileLabel = new QLabel(tr("Suppression File:"), this);

    QLabel *suppressionsLabel = new QLabel(tr("Suppression:"), this);
    suppressionsLabel->setBuddy(m_suppressionEdit);

    QFont font;
    font.setFamily(QLatin1String("Monospace"));
    m_suppressionEdit->setFont(font);

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Save);

    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->addRow(fileLabel, m_fileChooser);
    formLayout->addRow(suppressionsLabel);
    formLayout->addRow(m_suppressionEdit);
    formLayout->addRow(m_buttonBox);

    QFile defaultSuppFile(view->defaultSuppressionFile());
    if (!defaultSuppFile.exists()) {
        if (defaultSuppFile.open(QIODevice::WriteOnly)) {
            defaultSuppFile.close();
            m_cleanupIfCanceled = true;
        }
    }

    m_fileChooser->setExpectedKind(Utils::PathChooser::File);
    m_fileChooser->setPath(defaultSuppFile.fileName());
    m_fileChooser->setPromptDialogFilter(QLatin1String("*.supp"));
    m_fileChooser->setPromptDialogTitle(tr("Select Suppression File"));

    QString suppressions;
    foreach (const Error &error, m_errors)
        suppressions += suppressionText(error);

    m_suppressionEdit->setPlainText(suppressions);

    connect(m_fileChooser, SIGNAL(validChanged()), SLOT(validate()));
    connect(m_suppressionEdit->document(), SIGNAL(contentsChanged()), SLOT(validate()));
    connect(m_buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

void SuppressionDialog::maybeShow(MemcheckErrorView *view)
{
    QModelIndexList indices = view->selectionModel()->selectedRows();
    // Can happen when using arrow keys to navigate and shortcut to trigger suppression:
    if (indices.isEmpty() && view->selectionModel()->currentIndex().isValid())
        indices.append(view->selectionModel()->currentIndex());

    QList<XmlProtocol::Error> errors;
    foreach (const QModelIndex &index, indices) {
        Error error = view->model()->data(index, ErrorListModel::ErrorRole).value<Error>();
        if (!error.suppression().isNull())
            errors.append(error);
    }

    if (errors.isEmpty())
        return;

    SuppressionDialog dialog(view, errors);
    dialog.exec();
}

void SuppressionDialog::accept()
{
    const QString path = m_fileChooser->path();
    QTC_ASSERT(!path.isEmpty(), return);
    QTC_ASSERT(!m_suppressionEdit->toPlainText().trimmed().isEmpty(), return);

    Utils::FileSaver saver(path, QIODevice::Append);
    QTextStream stream(saver.file());
    stream << m_suppressionEdit->toPlainText();
    saver.setResult(&stream);
    if (!saver.finalize(this))
        return;

    // Add file to project if there is a project containing this file on the file system.
    ProjectExplorer::SessionManager *session = ProjectExplorer::ProjectExplorerPlugin::instance()->session();
    if (!session->projectForFile(path)) {
        foreach (ProjectExplorer::Project *p, session->projects()) {
            if (path.startsWith(p->projectDirectory())) {
                p->rootProjectNode()->addFiles(ProjectExplorer::UnknownFileType, QStringList() << path);
                break;
            }
        }
    }

    m_settings->subConfig<ValgrindBaseSettings>()->addSuppressionFiles(QStringList(path));

    QModelIndexList indices = m_view->selectionModel()->selectedRows();
    qSort(indices.begin(), indices.end(), sortIndizesReverse);
    QAbstractItemModel *model = m_view->model();
    foreach (const QModelIndex &index, indices) {
        bool removed = model->removeRow(index.row());
        QTC_ASSERT(removed, qt_noop());
        Q_UNUSED(removed);
    }

    // One suppression might hide multiple rows, care for that.
    for (int row = 0; row < model->rowCount(); ++row ) {
        const Error rowError = model->data(
            model->index(row, 0), ErrorListModel::ErrorRole).value<Error>();

        foreach (const Error &error, m_errors) {
            if (equalSuppression(rowError, error)) {
                bool removed = model->removeRow(row);
                QTC_CHECK(removed);
                // Gets incremented in the for loop again.
                --row;
                break;
            }
        }
    }

    // Select a new item.
    m_view->setCurrentIndex(indices.first());

    QDialog::accept();
}

void SuppressionDialog::reject()
{
    if (m_cleanupIfCanceled)
        QFile::remove(m_view->defaultSuppressionFile());

    QDialog::reject();
}

void SuppressionDialog::validate()
{
    bool valid = m_fileChooser->isValid()
            && !m_suppressionEdit->toPlainText().trimmed().isEmpty();

    m_buttonBox->button(QDialogButtonBox::Save)->setEnabled(valid);
}

} // namespace Internal
} // namespace Valgrind
