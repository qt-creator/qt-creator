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

#ifndef REWRITERVIEW_H
#define REWRITERVIEW_H

#include "qmldesignercorelib_global.h"
#include "abstractview.h"
#include "exception.h"
#include <modelnodepositionstorage.h>
#include <QMap>
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

class RewriterError {
public:
    enum Type {
        NoError = 0,
        InternalError = 1,
        ParseError = 2
    };

public:
    RewriterError();
    RewriterError(const QmlJS::DiagnosticMessage &qmlError, const QUrl &document);
    RewriterError(const QString &shortDescription);
    RewriterError(Exception *exception);

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

class QMLDESIGNERCORE_EXPORT RewriterView : public AbstractView
{
    Q_OBJECT

public:
    enum DifferenceHandling {
        Validate,
        Amend
    };

public:
    RewriterView(DifferenceHandling differenceHandling, QObject *parent);
    ~RewriterView();

    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void nodeCreated(const ModelNode &createdNode) override;
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;
    void propertiesRemoved(const QList<AbstractProperty>& propertyList) override;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty>& propertyList,PropertyChangeFlags propertyChange) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) override;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<ModelNode> &nodeList,
                            const QList<QVariant> &data) override;

    void rewriterBeginTransaction() override;
    void rewriterEndTransaction() override;

    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;

    TextModifier *textModifier() const;
    void setTextModifier(TextModifier *textModifier);
    QString textModifierContent() const;

    void reactivateTextMofifierChangeSignals();
    void deactivateTextMofifierChangeSignals();

    Internal::ModelNodePositionStorage *positionStorage() const
    { return m_positionStorage; }

    QList<RewriterError> errors() const;
    void clearErrors();
    void setErrors(const QList<RewriterError> &errors);
    void addError(const RewriterError &error);

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

    QStringList importDirectories() const;

    QSet<QPair<QString, QString> > qrcMapping() const;

signals:
    void errorsChanged(const QList<RewriterError> &errors);

public slots:
    void qmlTextChanged();
    void delayedSetup();

protected: // functions
    void importAdded(const Import &import);
    void importRemoved(const Import &import);

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
    QList<RewriterError> m_errors;
    int transactionLevel;
    RewriterTransaction m_removeDefaultPropertyTransaction;
    QString m_rewritingErrorMessage;
    QString lastCorrectQmlSource;
    bool m_checkErrors;
};

} //QmlDesigner

#endif // REWRITERVIEW_H
