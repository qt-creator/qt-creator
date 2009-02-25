/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef STACKEDEDITORGROUP_H
#define STACKEDEDITORGROUP_H

#include "editorgroup.h"

#include <QtCore/QMap>
#include <QtGui/QSortFilterProxyModel>

QT_BEGIN_NAMESPACE
class QComboBox;
class QToolBar;
class QToolButton;
class QLabel;
class QStackedWidget;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

class StackedEditorGroup : public EditorGroup
{
    Q_OBJECT

public:
    StackedEditorGroup(QWidget *parent = 0);
    virtual ~StackedEditorGroup();

    //EditorGroup
    int editorCount() const;
    void addEditor(IEditor *editor);
    void insertEditor(int i, IEditor *editor);
    void removeEditor(IEditor *editor);
    IEditor *currentEditor() const;
    void setCurrentEditor(IEditor *editor);
    QList<IEditor *> editors() const;
    void showEditorInfoBar(const QString &kind,
                           const QString &infoText,
                           const QString &buttonText,
                           QObject *object, const char *member);
    void hideEditorInfoBar(const QString &kind);

    void focusInEvent(QFocusEvent *e);

protected:
    QList<IEditor *> editorsInNaturalOrder() const;

private slots:
    void sendCloseRequest();
    void updateEditorStatus(Core::IEditor *editor = 0);
    void checkEditorStatus();
    void setEditorFocus(int index);
    void makeEditorWritable();
    void listSelectionChanged(int index);

private:
    void updateToolBar(IEditor *editor);
    int indexOf(IEditor *editor);
    void checkProjectLoaded(IEditor *editor);

    QWidget *m_toplevel;
    QWidget *m_toolBar;
    QToolBar *m_activeToolBar;
    QStackedWidget *m_container;
    QComboBox *m_editorList;
    QToolButton *m_closeButton;
    QToolButton *m_lockButton;
    QToolBar *m_defaultToolBar;
    QString m_infoWidgetKind;
    QFrame *m_infoWidget;
    QLabel *m_infoWidgetLabel;
    QToolButton *m_infoWidgetButton;
    IEditor *m_editorForInfoWidget;
    QSortFilterProxyModel m_proxyModel;
    QMap<QWidget *, IEditor *> m_widgetEditorMap;
};

} // namespace Internal
} // namespace Core

#endif // STACKEDEDITORGROUP_H
