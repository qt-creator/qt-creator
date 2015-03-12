/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DIFFEDITOR_H
#define DIFFEDITOR_H

#include "diffeditorcontroller.h"

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QSpinBox;
class QToolBar;
class QToolButton;
class QStackedWidget;
QT_END_NAMESPACE

namespace TextEditor { class TextEditorWidget; }

namespace DiffEditor {

namespace Internal {
class DescriptionEditorWidget;
class DiffEditorDocument;
class IDiffView;

class DiffEditor : public Core::IEditor
{
    Q_OBJECT

public:
    DiffEditor(const QSharedPointer<DiffEditorDocument> &doc);
    ~DiffEditor();

public:
    Core::IEditor *duplicate();

    bool open(QString *errorString,
              const QString &fileName,
              const QString &realFileName);
    Core::IDocument *document();

    QWidget *toolBar();

private slots:
    void documentHasChanged();
    void toggleDescription();
    void updateDescription();
    void contextLineCountHasChanged(int lines);
    void ignoreWhitespaceHasChanged();
    void prepareForReload();
    void reloadHasFinished(bool success);
    void setCurrentDiffFileIndex(int index);
    void documentStateChanged();

    void toggleSync();

private:
    void loadSettings();
    void saveSetting(const QString &key, const QVariant &value) const;
    void updateEntryToolTip();
    void showDiffView(IDiffView *view);
    void updateDiffEditorSwitcher();
    void addView(IDiffView *view);
    IDiffView *currentView() const;
    void setCurrentView(IDiffView *view);
    IDiffView *nextView();
    void setupView(IDiffView *view);

    QSharedPointer<DiffEditorDocument> m_document;
    DescriptionEditorWidget *m_descriptionWidget;
    QStackedWidget *m_stackedWidget;
    QVector<IDiffView *> m_views;
    QToolBar *m_toolBar;
    QComboBox *m_entriesComboBox;
    QSpinBox *m_contextSpinBox;
    QAction *m_toggleSyncAction;
    QAction *m_whitespaceButtonAction;
    QAction *m_toggleDescriptionAction;
    QAction *m_reloadAction;
    QLabel *m_contextLabel;
    QAction *m_viewSwitcherAction;
    QPair<QString, QString> m_currentFileChunk;
    int m_currentViewIndex;
    int m_currentDiffFileIndex;
    int m_ignoreChanges;
    bool m_sync;
    bool m_showDescription;
};

} // namespace Internal
} // namespace DiffEditor

#endif // DIFFEDITOR_H
