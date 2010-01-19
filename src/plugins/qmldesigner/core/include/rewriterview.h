/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef REWRITERVIEW_H
#define REWRITERVIEW_H

#include "corelib_global.h"
#include "abstractview.h"
#include "exception.h"
#include <modelnodepositionstorage.h>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QPointer>
#include <QWeakPointer>
#include <QtCore/QHash>

#include <modelnode.h>
#include <QScopedPointer>


namespace QmlDesigner {

class CORESHARED_EXPORT TextModifier;

namespace Internal {

class TextToModelMerger;
class ModelToTextMerger;
class ModelNodePositionStorage;

} //Internal


class CORESHARED_EXPORT RewriterView : public AbstractView
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
        Error(const QmlError &qmlError);
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
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);
    void nodeTypeChanged(const ModelNode &node,const QString &type, int majorVersion, int minorVersion);
    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);

    void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl);

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList, const QList<ModelNode> &lastSelectedNodeList);

    TextModifier *textModifier() const;
    void setTextModifier(TextModifier *textModifier);

    Internal::ModelNodePositionStorage *positionStorage() const
    { return m_positionStorage; }

    QList<Error> errors() const;
    void clearErrors();
    void setErrors(const QList<Error> &errors);
    void addError(const Error &error);

    QMap<ModelNode, QString> extractText(const QList<ModelNode> &nodes) const;
    int nodeOffset(const ModelNode &node) const;
    int nodeLength(const ModelNode &node) const;
    int firstDefinitionInsideOffset(const ModelNode &node) const;
    int firstDefinitionInsideLength(const ModelNode &node) const;

signals:
    void errorsChanged(const QList<RewriterView::Error> &errors);

public slots:
    void qmlTextChanged();

protected: // functions
    Internal::ModelToTextMerger *modelToTextMerger() const;
    Internal::TextToModelMerger *textToModelMerger() const;
    bool isModificationGroupActive() const;
    void setModificationGroupActive(bool active);
    void applyModificationGroupChanges();
    void setupComponent(const ModelNode &node);

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
};

} //QmlDesigner

#endif // REWRITERVIEW_H
