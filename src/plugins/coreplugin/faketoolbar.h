/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef FAKETOOLBAR_H
#define FAKETOOLBAR_H

#include <QWidget>
#include <QtCore/QPointer>

namespace Core {
    class IEditor;
    class OpenEditorsModel;
}

QT_BEGIN_NAMESPACE
class QComboBox;
class QToolButton;
class QToolBar;
QT_END_NAMESPACE

namespace Designer {
namespace Internal {

/**
  * Fakes an IEditor-like toolbar for design mode widgets such as Qt Designer and Bauhaus.
  * Creates a combobox for open files and lock and close buttons on the right.
  */
class FakeToolBar : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(FakeToolBar)
public:
    explicit FakeToolBar(QWidget *toolbar, QWidget *parent = 0);

    void updateActions();

private slots:
    void editorChanged(Core::IEditor *newSelection);
    void close();
    void listSelectionActivated(int row);
    void listContextMenu(QPoint);
    void makeEditorWritable();
    void updateEditorStatus();

private:
    Core::OpenEditorsModel *m_editorsListModel;
    QComboBox *m_editorList;
    QToolButton *m_closeButton;
    QToolButton *m_lockButton;
    QAction *m_goBackAction;
    QAction *m_goForwardAction;
    QPointer<Core::IEditor> m_editor;
};

}
}

#endif // FAKETOOLBAR_H
