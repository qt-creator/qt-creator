// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    RewriterView(DifferenceHandling, ExternalDependenciesInterface &externalDependencies)
        : AbstractView{externalDependencies}
    {}
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
    void nodeOrderChanged(const NodeListProperty &) override {}
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

    void auxiliaryDataChanged([[maybe_unused]] const ModelNode &node,
                              [[maybe_unused]] AuxiliaryDataKeyView key,
                              [[maybe_unused]] const QVariant &data) override
    {}

    Internal::ModelNodePositionStorage *positionStorage() const { return nullptr; }

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
