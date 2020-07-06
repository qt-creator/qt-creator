/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qmldesignercorelib_global.h"
#include "abstractview.h"

namespace QmlJS {
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

struct CppTypeData
{
    QString superClassName;
    QString importUrl;
    QString versionString;
    QString cppClassName;
    QString typeName;
    bool isSingleton = false;
};

class RewriterView : public AbstractView
{
    Q_OBJECT

public:
    enum DifferenceHandling {
        Validate,
        Amend
    };

public:
    RewriterView(DifferenceHandling, QObject *) {}
    ~RewriterView() override {}

    void modelAttached(Model *) override {}
    void modelAboutToBeDetached(Model *) override {}
    void nodeCreated(const ModelNode &) override {}
    void nodeRemoved(const ModelNode &, const NodeAbstractProperty &, PropertyChangeFlags) override
    {}
    void propertiesAboutToBeRemoved(const QList<AbstractProperty> &) override {}
    void propertiesRemoved(const QList<AbstractProperty> &) override {}
    void variantPropertiesChanged(const QList<VariantProperty> &, PropertyChangeFlags) override {}
    void bindingPropertiesChanged(const QList<BindingProperty> &, PropertyChangeFlags) override {}
    void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &,
                                        PropertyChangeFlags) override
    {}
    void nodeReparented(const ModelNode &,
                        const NodeAbstractProperty &,
                        const NodeAbstractProperty &,
                        AbstractView::PropertyChangeFlags) override
    {}
    void nodeIdChanged(const ModelNode &, const QString &, const QString &) override {}
    void nodeOrderChanged(const NodeListProperty &, const ModelNode &, int) override {}
    void rootNodeTypeChanged(const QString &, int, int) override {}
    void nodeTypeChanged(const ModelNode &, const TypeName &, int, int) override {}
    void customNotification(const AbstractView *,
                            const QString &,
                            const QList<ModelNode> &,
                            const QList<QVariant> &) override
    {}

    void rewriterBeginTransaction() override {}
    void rewriterEndTransaction() override {}

    void importsChanged(const QList<Import> &, const QList<Import> &) override {}

    TextModifier *textModifier() const { return {}; }
    void setTextModifier(TextModifier *) {}
    QString textModifierContent() const { return {}; }

    void reactivateTextMofifierChangeSignals() {}
    void deactivateTextMofifierChangeSignals() {}

    void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data) override
    {}

    Internal::ModelNodePositionStorage *positionStorage() const {}

    QList<DocumentMessage> warnings() const { return {}; }
    QList<DocumentMessage> errors() const { return {}; }
    void clearErrorAndWarnings() {}
    void setErrors(const QList<DocumentMessage> &) {}
    void setWarnings(const QList<DocumentMessage> &) {}
    void setIncompleteTypeInformation(bool) {}
    bool hasIncompleteTypeInformation() const { return false; }
    void addError(const DocumentMessage &) {}

    void enterErrorState(const QString &) {}
    bool inErrorState() const { return false; }
    void leaveErrorState() {}
    void resetToLastCorrectQml() {}

    QMap<ModelNode, QString> extractText(const QList<ModelNode> &) const;
    int nodeOffset(const ModelNode &) const;
    int nodeLength(const ModelNode &) const;
    int firstDefinitionInsideOffset(const ModelNode &) const { return {}; }
    int firstDefinitionInsideLength(const ModelNode &) const { return {}; }
    bool modificationGroupActive() { return {}; }
    ModelNode nodeAtTextCursorPosition(int) const { return {}; }

    bool renameId(const QString &, const QString &) { return {}; }

    const QmlJS::Document *document() const { return {}; }
    const QmlJS::ScopeChain *scopeChain() const { return {}; }

    QString convertTypeToImportAlias(const QString &) const { return {}; }

    bool checkSemanticErrors() const { return {}; }

    void setCheckSemanticErrors(bool) {}

    QString pathForImport(const Import &) { return {}; }

    QStringList importDirectories() const { return {}; }

    QSet<QPair<QString, QString>> qrcMapping() const { return {}; }

    void moveToComponent(const ModelNode &) {}

    QStringList autoComplete(const QString &, int, bool = true) { return {}; }

    QList<CppTypeData> getCppTypes() { return {}; }

    void setWidgetStatusCallback(std::function<void(bool)> setWidgetStatusCallback);

    void qmlTextChanged() {}
    void delayedSetup() {}

    void writeAuxiliaryData() {}
    void restoreAuxiliaryData() {}

    QString getRawAuxiliaryData() const { return {}; }
    QString auxiliaryDataAsQML() const { return {}; }

    ModelNode getNodeForCanonicalIndex(int) { return {}; }
};

} //QmlDesigner
