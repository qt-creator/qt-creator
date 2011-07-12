/**************************************************************************
**
** This file is part of Qt Creator Instrumentation Tools
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "suppressiondialog.h"
#include "ui_suppressiondialog.h"

#include "memcheckerrorview.h"
#include "valgrindsettings.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <utils/pathchooser.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QtCore/QFile>
#include <QtGui/QPushButton>

#include <valgrind/xmlprotocol/suppression.h>
#include <valgrind/xmlprotocol/errorlistmodel.h>
#include <valgrind/xmlprotocol/stack.h>
#include <valgrind/xmlprotocol/frame.h>

using namespace Analyzer;
using namespace Valgrind::XmlProtocol;

namespace {

QString suppressionText(const Error &error)
{
    Suppression sup(error.suppression());

    // workaround: https://bugs.kde.org/show_bug.cgi?id=255822
    if (sup.frames().size() >= 24) {
        sup.setFrames(sup.frames().mid(0, 23));
    }
    QTC_ASSERT(sup.frames().size() < 24, qt_noop());

    // try to set some useful name automatically, instead of "insert_name_here"
    // we take the last stack frame and append the suppression kind, e.g.:
    // QDebug::operator<<(bool) [Memcheck:Cond]
    if (!error.stacks().isEmpty() && !error.stacks().first().frames().isEmpty()) {
        const Frame &frame = error.stacks().first().frames().first();

        QString newName;
        if (!frame.functionName().isEmpty())
            newName = frame.functionName();
        else if (!frame.object().isEmpty())
            newName = frame.object();

        if (!newName.isEmpty()) {
            sup.setName(newName + '[' + sup.kind() + ']');
        }
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

    for (int i = 0; i < frames; ++i) {
        if (errorFrames.at(i) != suppressedFrames.at(i))
            return false;
    }

    return true;
}

bool sortIndizesReverse(const QModelIndex &l, const QModelIndex &r)
{
    return l.row() > r.row();
}

} // namespace anoe

namespace Valgrind {
namespace Internal {

SuppressionDialog::SuppressionDialog(MemcheckErrorView *view, QWidget *parent, Qt::WindowFlags f)
  : QDialog(parent, f),
    m_view(view),
    m_ui(new Ui::SuppressionDialog),
    m_settings(view->settings()),
    m_cleanupIfCanceled(false)
{
    m_ui->setupUi(this);

    ///NOTE: pathchooser requires existing files...
    QFile defaultSuppFile(view->defaultSuppressionFile());
    if (!defaultSuppFile.exists()) {
        if (defaultSuppFile.open(QIODevice::WriteOnly)) {
            defaultSuppFile.close();
            m_cleanupIfCanceled = true;
        }
    }

    //NOTE: first set kind, then set path since otherwise the file will be seen as "invalid"
    m_ui->fileChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui->fileChooser->setPath(defaultSuppFile.fileName());
    m_ui->fileChooser->setPromptDialogFilter(QLatin1String("*.supp"));
    m_ui->fileChooser->setPromptDialogTitle(tr("Select Suppression File"));
    connect(m_ui->fileChooser, SIGNAL(validChanged()),
            SLOT(validate()));
    connect(m_ui->suppressionEdit->document(), SIGNAL(contentsChanged()),
            SLOT(validate()));

    QString suppressions;
    QModelIndexList indizes = m_view->selectionModel()->selectedRows();
    if (indizes.isEmpty() && m_view->selectionModel()->currentIndex().isValid()) {
        // can happen when using arrow keys to navigate and shortcut to trigger suppression
        indizes << m_view->selectionModel()->currentIndex();
    }
    foreach (const QModelIndex &index, indizes) {
        Error error = m_view->model()->data(index, ErrorListModel::ErrorRole).value<Error>();
        if (!error.suppression().isNull())
            m_errors << error;
    }

    foreach (const Error &error, m_errors)
        suppressions += suppressionText(error);

    m_ui->suppressionEdit->setPlainText(suppressions);

    setWindowTitle(tr("Save Suppression"));
}

bool SuppressionDialog::shouldShow() const
{
    return !m_errors.isEmpty();
}

void SuppressionDialog::accept()
{
    const QString path = m_ui->fileChooser->path();
    QTC_ASSERT(!path.isEmpty(), return);
    QTC_ASSERT(!m_ui->suppressionEdit->toPlainText().trimmed().isEmpty(), return);

    Utils::FileSaver saver(path, QIODevice::Append);
    QTextStream stream(saver.file());
    stream << m_ui->suppressionEdit->toPlainText();
    saver.setResult(&stream);
    if (!saver.finalize(this))
        return;

    // add file to project (if there is a project that contains this file on the file system)
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

    QModelIndexList indizes = m_view->selectionModel()->selectedRows();
    qSort(indizes.begin(), indizes.end(), sortIndizesReverse);
    foreach (const QModelIndex &index, indizes) {
        bool removed = m_view->model()->removeRow(index.row());
        QTC_ASSERT(removed, qt_noop());
        Q_UNUSED(removed);
    }

    // one suppression might hide multiple rows, care for that
    for (int row = 0; row < m_view->model()->rowCount(); ++row ) {
        const Error rowError = m_view->model()->data(
            m_view->model()->index(row, 0), ErrorListModel::ErrorRole).value<Error>();

        foreach (const Error &error, m_errors) {
            if (equalSuppression(rowError, error)) {
                bool removed = m_view->model()->removeRow(row);
                QTC_ASSERT(removed, qt_noop());
                Q_UNUSED(removed);
                // gets increased in the for loop again
                --row;
                break;
            }
        }
    }

    // select a new item
    m_view->setCurrentIndex(indizes.first());

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
    bool valid = m_ui->fileChooser->isValid()
            && !m_ui->suppressionEdit->toPlainText().trimmed().isEmpty();

    m_ui->buttonBox->button(QDialogButtonBox::Save)->setEnabled(valid);
}

} // namespace Internal
} // namespace Valgrind
