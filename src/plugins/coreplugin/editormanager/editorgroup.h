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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef EDITORGROUP_H
#define EDITORGROUP_H

#include <coreplugin/icontext.h>

#include <QtCore/QEvent>
#include <QtCore/QAbstractListModel>

#include <QtGui/QFrame>
#include <QtGui/QAbstractButton>
#include <QtGui/QPen>

namespace Core {

class IEditor;
class EditorGroup;

namespace Internal {

class EditorModel;

// Also used by the EditorManager
class EditorGroupContext : public IContext
{
    Q_OBJECT

public:
    EditorGroupContext(EditorGroup *editorGroup);
    EditorGroup *editorGroup();
    // IContext
    QList<int> context() const;
    QWidget *widget();
private:
    QList<int> m_context;
    EditorGroup *m_editorGroup;
};

} // namespace Internal

class CORE_EXPORT EditorGroup : public QFrame
{
    Q_OBJECT

public:
    EditorGroup(QWidget *parent);
    virtual ~EditorGroup() {};

    virtual IContext *contextObject() { return m_contextObject; }
    virtual QWidget *widget() { return this; }

    virtual int editorCount() const = 0;
    virtual void addEditor(IEditor *editor);
    virtual void insertEditor(int i, IEditor *editor);
    virtual void removeEditor(IEditor *editor);
    virtual QList<IEditor*> editors() const = 0;

    virtual IEditor *currentEditor() const = 0;
    virtual void setCurrentEditor(IEditor *editor) = 0;

    virtual void moveEditorsFromGroup(EditorGroup *group);
    virtual void moveEditorFromGroup(EditorGroup *group, IEditor *editor);

    virtual QByteArray saveState() const;
    virtual bool restoreState(const QByteArray &state);

    virtual void showEditorInfoBar(const QString &kind,
                                   const QString &infoText,
                                   const QString &buttonText,
                                   QObject *object, const char *member);

    virtual void hideEditorInfoBar(const QString &kind);

    QSize minimumSizeHint() const;
    void focusInEvent(QFocusEvent *e);
    void focusOutEvent(QFocusEvent *e);
    void paintEvent(QPaintEvent *e);

signals:
    void closeRequested(Core::IEditor *editor);
    void editorRemoved(Core::IEditor *editor);
    void editorAdded(Core::IEditor *editor);

protected:
    virtual QList<IEditor *> editorsInNaturalOrder() const { return editors(); }
    Internal::EditorModel *model() const { return m_model; }

private:
    Internal::EditorGroupContext *m_contextObject;
    Internal::EditorModel *m_model;
};

namespace Internal {

// Also used by StackedEditorGroup
class EditorModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    EditorModel(QObject *parent) : QAbstractItemModel(parent) {}
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QModelIndex parent(const QModelIndex &/*index*/) const { return QModelIndex(); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const;

    void addEditor(IEditor *editor) { insertEditor(rowCount(), editor); }

    void insertEditor(int index, IEditor *editor);
    void removeEditor(IEditor *editor);
    void emitDataChanged(IEditor *editor);

    QList<IEditor *> editors() const { return m_editors; }
    QModelIndex indexOf(IEditor *editor) const;
    QModelIndex indexOf(const QString &filename) const;

private slots:
    void itemChanged();
private:
    QList<IEditor *> m_editors;
};

} // namespace Internal
} // namespace Core

#endif // EDITORGROUP_H
