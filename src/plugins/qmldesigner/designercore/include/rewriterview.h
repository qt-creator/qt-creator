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

#ifndef REWRITERVIEW_H
#define REWRITERVIEW_H

#include "qmldesignercorelib_global.h"
#include "abstractview.h"
#include "exception.h"
#include <modelnodepositionstorage.h>
#include <QHash>
#include <QMap>
#include <QPointer>
#include <QWeakPointer>
#include <QHash>
#include <QUrl>

#include <modelnode.h>
#include <QScopedPointer>

namespace QmlJS {

class DiagnosticMessage;
class LookupContext;
class Document;
class ScopeChain;
}


namespace QmlDesigner {

class TextModifier;

namespace Internal {

class TextToModelMerger;
class ModelToTextMerger;
class ModelNodePositionStorage;

} //Internal

class QMLDESIGNERCORE_EXPORT RewriterView : public AbstractView
{
    Q_OBJECT

public:
    enum DifferenceHandling {
        Validate,
        Amend
    };

    class Error {
    public:
        enum Type {
            NoError = 0,
            InternalError = 1,
            ParseError = 2
        };

    public:
        Error();
        Error(const QmlJS::DiagnosticMessage &qmlError, const QUrl &document);
        Error(const QString &shortDescription);
        Error(Exception *exception);

        Type type() const
        { return m_type; }

        int line() const
        { return m_line; }

        int column() const
        { return m_column; }

        QString description() const
        { return m_description; }

        QUrl url() const
        { return m_url; }

        QString toString() const;

    private:
        Type m_type;
        int m_line;
        int m_column;
        QString m_description;
        QUrl m_url;
    };

public:
    RewriterView(DifferenceHandling differenceHandling, QObject *parent);
    ~RewriterView();

    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);
    void nodeCreated(const ModelNode &createdNode);
    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange);
    void propertiesAdded(const ModelNode &node, const QList<AbstractProperty>& propertyList);
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);
    void propertiesRemoved(const QList<AbstractProperty>& propertyList);
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange);
    void nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);
    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);

    void instancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList);
    void instancesCompleted(const QVector<ModelNode> &completedNodeList);
    void instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash);
    void instancesRenderImageChanged(const QVector<ModelNode> &nodeList);
    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList);
    void instancesChildrenChanged(const QVector<ModelNode> &nodeList);
    void instancesToken(const QString &tokenName, int tokenNumber, const QVector<ModelNode> &nodeVector);

    void nodeSourceChanged(const ModelNode &modelNode, const QString &newNodeSource);

    void rewriterBeginTransaction();
    void rewriterEndTransaction();

    void actualStateChanged(const ModelNode &node);

    void importAdded(const Import &import);
    void importRemoved(const Import &import);
    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports);

    void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl);

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList, const QList<ModelNode> &lastSelectedNodeList);

    TextModifier *textModifier() const;
    void setTextModifier(TextModifier *textModifier);
    QString textModifierContent() const;

    void reactivateTextMofifierChangeSignals();
    void deactivateTextMofifierChangeSignals();

    Internal::ModelNodePositionStorage *positionStorage() const
    { return m_positionStorage; }

    QList<Error> errors() const;
    void clearErrors();
    void setErrors(const QList<Error> &errors);
    void addError(const Error &error);

    void enterErrorState(const QString &errorMessage);
    bool inErrorState() const { return !m_rewritingErrorMessage.isEmpty(); }
    void leaveErrorState() { m_rewritingErrorMessage.clear(); }
    void resetToLastCorrectQml();

    QMap<ModelNode, QString> extractText(const QList<ModelNode> &nodes) const;
    int nodeOffset(const ModelNode &node) const;
    int nodeLength(const ModelNode &node) const;
    int firstDefinitionInsideOffset(const ModelNode &node) const;
    int firstDefinitionInsideLength(const ModelNode &node) const;
    bool modificationGroupActive();
    ModelNode nodeAtTextCursorPosition(int cursorPosition) const;

    bool renameId(const QString& oldId, const QString& newId);

    const QmlJS::Document *document() const;
    const QmlJS::ScopeChain *scopeChain() const;

    QString convertTypeToImportAlias(const QString &type) const;

    bool checkSemanticErrors() const
    { return m_checkErrors; }

    void setCheckSemanticErrors(bool b)
    { m_checkErrors = b; }

    QString pathForImport(const Import &import);

    QWidget *widget();

signals:
    void errorsChanged(const QList<RewriterView::Error> &errors);

public slots:
    void qmlTextChanged();
    void delayedSetup();

protected: // functions
    Internal::ModelToTextMerger *modelToTextMerger() const;
    Internal::TextToModelMerger *textToModelMerger() const;
    bool isModificationGroupActive() const;
    void setModificationGroupActive(bool active);
    void applyModificationGroupChanges();
    void applyChanges();

private: //variables
    DifferenceHandling m_differenceHandling;
    bool m_modificationGroupActive;
    Internal::ModelNodePositionStorage *m_positionStorage;
    QScopedPointer<Internal::ModelToTextMerger> m_modelToTextMerger;
    QScopedPointer<Internal::TextToModelMerger> m_textToModelMerger;
    TextModifier *m_textModifier;
    QList<Error> m_errors;
    int transactionLevel;
    RewriterTransaction m_removeDefaultPropertyTransaction;
    QString m_rewritingErrorMessage;
    QString lastCorrectQmlSource;
    bool m_checkErrors;
};

} //QmlDesigner

#endif // REWRITERVIEW_H
