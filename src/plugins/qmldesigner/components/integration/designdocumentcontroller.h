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

#ifndef DesignDocumentController_h
#define DesignDocumentController_h

#include "rewriterview.h"

#include <QtCore/QObject>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE
class QUndoStack;
class QWidget;
class QIODevice;
class QProcess;
class QPlainTextEdit;
class QDeclarativeError;
QT_END_NAMESPACE

namespace QmlDesigner {

class Model;
class ModelNode;
class TextModifier;
class QmlObjectNode;
class RewriterView;
class ItemLibrary;
class NavigatorView;
class AllPropertiesBox;
class StatesEditorWidget;
class FormEditorView;

class DesignDocumentController: public QObject
{
    Q_OBJECT

public:
    DesignDocumentController(QObject *parent);
    ~DesignDocumentController();

    QString displayName() const;
    QString fileName() const;
    void setFileName(const QString &fileName);

    QList<RewriterView::Error> loadMaster(QPlainTextEdit *edit);
    QList<RewriterView::Error> loadMaster(const QByteArray &qml);
    void loadCurrentModel();
    void close();

    bool isDirty() const;
    bool isUndoAvailable() const;
    bool isRedoAvailable() const;

    Model *model() const;
    Model *masterModel() const;    

    RewriterView *rewriterView() const;    

    bool isModelSyncBlocked() const;
    void blockModelSync(bool block);

    QString contextHelpId() const;
    QList<RewriterView::Error> qmlErrors() const;

    void setItemLibrary(ItemLibrary* itemLibrary);
    void setNavigator(NavigatorView* navigatorView);
    void setAllPropertiesBox(AllPropertiesBox* allPropertiesBox);
    void setStatesEditorWidget(StatesEditorWidget* statesEditorWidget);
    void setFormEditorView(FormEditorView *formEditorView);

signals:
    void displayNameChanged(const QString &newFileName);
    void dirtyStateChanged(bool newState);

    void undoAvailable(bool isAvailable);
    void redoAvailable(bool isAvailable);
    void designDocumentClosed();
    void qmlErrorsChanged(const QList<RewriterView::Error> &errors);

    void fileToOpen(const QString &path);

public slots:
    bool save(QWidget *parent = 0);
    void saveAs(QWidget *parent = 0);
    void deleteSelected();
    void copySelected();
    void cutSelected();
    void paste();
    void selectAll();
    void undo();
    void redo();

#ifdef ENABLE_TEXT_VIEW
    void showText();
    void showForm();
#endif // ENABLE_TEXT_VIEW

private slots:
    void doRealSaveAs(const QString &fileName);
    void showError(const QString &message, QWidget *parent = 0) const;
    void changeCurrentModelTo(const ModelNode &node);

private:
    QWidget *centralWidget() const;
    class DesignDocumentControllerPrivate *m_d;
    bool save(QIODevice *device, QString *errorMessage);
};

} // namespace QmlDesigner

#endif // DesignDocumentController_h
