/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef SUBMITEDITORWIDGET_H
#define SUBMITEDITORWIDGET_H

#include "utils_global.h"

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QListWidgetItem;
class QAction;
QT_END_NAMESPACE

namespace Core {
namespace Utils {

struct SubmitEditorWidgetPrivate;

/* The submit editor presents the commit message in a text editor and an
 * checkable list of modified files in a list window. The user can delete
 * files from the list by pressing unchecking them or diff the selection
 * by doubleclicking.
 *
 * Additionally, standard creator actions  can be registered:
 * Undo/redo will be set up to work with the description editor.
 * Submit will be set up to be enabled according to checkstate.
 * Diff will be set up to trigger diffSelected().
 *
 * Note that the actions are connected by signals; in the rare event that there
 * are several instances of the SubmitEditorWidget belonging to the same
 * context active, the actions must be registered/unregistered in the editor
 * change event.
 * Care should be taken to ensure the widget is deleted properly when the
 * editor closes. */

class QWORKBENCH_UTILS_EXPORT SubmitEditorWidget : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(SubmitEditorWidget)
    Q_PROPERTY(QString descriptionText READ descriptionText WRITE setDescriptionText DESIGNABLE true)
    Q_PROPERTY(QStringList fileList READ fileList WRITE setFileList DESIGNABLE true)
public:
    explicit SubmitEditorWidget(QWidget *parent = 0);
    virtual ~SubmitEditorWidget();

    void registerActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                         QAction *submitAction = 0, QAction *diffAction = 0);
    void unregisterActions(QAction *editorUndoAction,  QAction *editorRedoAction,
                           QAction *submitAction = 0, QAction *diffAction = 0);

    QString descriptionText() const;
    void setDescriptionText(const QString &text);
    // Should be used to normalize newlines.
    QString trimmedDescriptionText() const;

    // The raw file list
    QStringList fileList() const;
    void addFiles(const QStringList&, bool checked = true, bool userCheckable = true);
    void setFileList(const QStringList&);

    // Files to be included in submit
    QStringList checkedFiles() const;

    // Selected files for diff
    QStringList selectedFiles() const;

    QPlainTextEdit *descriptionEdit() const;

signals:
    void diffSelected(const QStringList &);
    void fileSelectionChanged(bool someFileSelected);
    void fileCheckStateChanged(bool someFileChecked);

protected:
    virtual void changeEvent(QEvent *e);
    void insertTopWidget(QWidget *w);

private slots:
    void triggerDiffSelected();
    void fileItemChanged(QListWidgetItem *);
    void fileSelectionChanged();

private:
    void addFilesUnblocked(const QStringList &list, bool checked, bool userCheckable);

    SubmitEditorWidgetPrivate *m_d;
};

} // namespace Utils
} // namespace Core

#endif // SUBMITEDITORWIDGET_H
