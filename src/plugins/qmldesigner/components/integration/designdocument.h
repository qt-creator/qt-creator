/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#ifndef DesignDocument_h
#define DesignDocument_h

#include <model.h>
#include <rewriterview.h>
#include <basetexteditmodifier.h>
#include <componenttextmodifier.h>
#include <subcomponentmanager.h>

#include <QObject>
#include <QString>

#include <QStackedWidget>

QT_BEGIN_NAMESPACE
class QUndoStack;
class QWidget;
class QIODevice;
class QProcess;
class QPlainTextEdit;
class QDeclarativeError;
QT_END_NAMESPACE

namespace QmlDesigner {

class ModelNode;
class TextModifier;
class QmlObjectNode;
class CrumbleBarInfo;
class ViewManager;
class QmlModelView;

class QMLDESIGNERCORE_EXPORT DesignDocument: public QObject
{
    Q_OBJECT
public:
    DesignDocument(QObject *parent = 0);
    ~DesignDocument();

    QString displayName() const;
    QString simplfiedDisplayName() const;

    void loadDocument(QPlainTextEdit *edit);
    void activateDocumentModel();
    void close();
    void updateSubcomponentManager();

    bool isUndoAvailable() const;
    bool isRedoAvailable() const;

    Model *currentModel() const;
    Model *documentModel() const;

    QString contextHelpId() const;
    QList<RewriterView::Error> qmlSyntaxErrors() const;
    bool hasQmlSyntaxErrors() const;

    RewriterView *rewriterView() const;

    void setEditor(Core::IEditor *editor);
    Core::IEditor *editor() const;

    TextEditor::ITextEditor *textEditor() const;
    QPlainTextEdit *plainTextEdit() const;
    QString fileName() const;
    int qtVersionId() const; // maybe that is not working, because the id should be not cached!!!
    bool isDocumentLoaded() const;

    void resetToDocumentModel();

    void goIntoSelectedComponent();

    void changeToDocumentModel();

signals:
    void displayNameChanged(const QString &newFileName);
    void dirtyStateChanged(bool newState);

    void undoAvailable(bool isAvailable);
    void redoAvailable(bool isAvailable);
    void designDocumentClosed();
    void qmlErrorsChanged(const QList<RewriterView::Error> &errors);

public slots:
    void deleteSelected();
    void copySelected();
    void cutSelected();
    void paste();
    void selectAll();
    void undo();
    void redo();
    void updateActiveQtVersion();
    void changeToSubComponentAndPushOnCrumblePath(const ModelNode &componentNode);
    void changeToSubComponent(const ModelNode &componentNode);
    void changeToExternalSubComponent(const QString &m_oldFileName);

private slots:
    void updateFileName(const QString &oldFileName, const QString &newFileName);

private: // functions
    void changeToInFileComponentModel();
    void activateCurrentModel(TextModifier *textModifier);

    QWidget *centralWidget() const;
    QString pathToQt() const;

    const ViewManager &viewManager() const;
    ViewManager &viewManager();

    ModelNode rootModelNode() const;

    bool loadInFileComponent(const ModelNode &componentNode);

    QmlModelView *qmlModelView();

private: // variables
    QScopedPointer<QStackedWidget> m_stackedWidget;
    QScopedPointer<Model> m_documentModel;
    QScopedPointer<Model> m_inFileComponentModel;
    QWeakPointer<Model> m_currentModel;
    QWeakPointer<Core::IEditor> m_textEditor;
    QScopedPointer<BaseTextEditModifier> m_documentTextModifier;
    QScopedPointer<ComponentTextModifier> m_inFileComponentTextModifier;
    QScopedPointer<SubComponentManager> m_subComponentManager;

    QScopedPointer<RewriterView> m_rewriterView;

    bool m_documentLoaded;
    int m_qtVersionId;
};

} // namespace QmlDesigner


#endif // DesignDocument_h
