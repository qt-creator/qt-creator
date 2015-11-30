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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
class QWidget;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Kit;
}

namespace QmlDesigner {

class ModelNode;
class TextModifier;
class QmlObjectNode;
class CrumbleBarInfo;
class ViewManager;
class AbstractView;

class QMLDESIGNERCORE_EXPORT DesignDocument: public QObject
{
    Q_OBJECT
public:
    DesignDocument(QObject *parent = 0);
    ~DesignDocument();

    QString displayName() const;
    QString simplfiedDisplayName() const;

    void loadDocument(QPlainTextEdit *edit);
    void attachRewriterToModel();
    void close();
    void updateSubcomponentManager();

    bool isUndoAvailable() const;
    bool isRedoAvailable() const;

    Model *currentModel() const;
    Model *documentModel() const;

    QString contextHelpId() const;
    QList<RewriterError> qmlSyntaxErrors() const;
    bool hasQmlSyntaxErrors() const;

    RewriterView *rewriterView() const;

    void setEditor(Core::IEditor *editor);
    Core::IEditor *editor() const;

    TextEditor::BaseTextEditor *textEditor() const;
    QPlainTextEdit *plainTextEdit() const;
    Utils::FileName fileName() const;
    ProjectExplorer::Kit *currentKit() const;
    bool isDocumentLoaded() const;

    void resetToDocumentModel();

    void changeToDocumentModel();

signals:
    void displayNameChanged(const QString &newFileName);
    void dirtyStateChanged(bool newState);

    void undoAvailable(bool isAvailable);
    void redoAvailable(bool isAvailable);
    void designDocumentClosed();
    void qmlErrorsChanged(const QList<RewriterError> &errors);

public slots:
    void deleteSelected();
    void copySelected();
    void cutSelected();
    void paste();
    void selectAll();
    void undo();
    void redo();
    void updateActiveQtVersion();
    void changeToSubComponent(const ModelNode &componentNode);
    void changeToMaster();

private slots:
    void updateFileName(const Utils::FileName &oldFileName, const Utils::FileName &newFileName);

private: // functions
    void changeToInFileComponentModel(ComponentTextModifier *textModifer);

    QWidget *centralWidget() const;
    QString pathToQt() const;

    const ViewManager &viewManager() const;
    ViewManager &viewManager();

    ModelNode rootModelNode() const;

    bool loadInFileComponent(const ModelNode &componentNode);

    AbstractView *view() const;

    Model *createInFileComponentModel();

private: // variables
    QScopedPointer<Model> m_documentModel;
    QScopedPointer<Model> m_inFileComponentModel;
    QPointer<Core::IEditor> m_textEditor;
    QScopedPointer<BaseTextEditModifier> m_documentTextModifier;
    QScopedPointer<ComponentTextModifier> m_inFileComponentTextModifier;
    QScopedPointer<SubComponentManager> m_subComponentManager;

    QScopedPointer<RewriterView> m_rewriterView;

    bool m_documentLoaded;
    ProjectExplorer::Kit *m_currentKit;
};

} // namespace QmlDesigner


#endif // DesignDocument_h
