// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef STYLESHEETMERGER_H
#define STYLESHEETMERGER_H

#include "qmldesignercorelib_global.h"

#include "utils/filepath.h"

#include <QString>
#include <QHash>
#include <modelnode.h>

namespace QmlDesigner {

class AbstractView;

struct ReparentInfo {
    QString generatedId;
    QString templateId;
    QString templateParentId;
    int parentIndex;
    bool alreadyReparented;
};

class QMLDESIGNERCORE_EXPORT StylesheetMerger
{
public:
    StylesheetMerger(AbstractView*, AbstractView*);
    void merge();
    static void styleMerge(const Utils::FilePath &templateFile,
                           Model *model,
                           class ExternalDependenciesInterface &ed);
    static void styleMerge(const QString &qmlTemplateString,
                           Model *model,
                           class ExternalDependenciesInterface &externalDependencies);

private:
    void preprocessStyleSheet();
    bool idExistsInBothModels(const QString& id);
    void replaceNode(ModelNode&, ModelNode&);
    void replaceRootNode(ModelNode& templateRootNode);
    void applyStyleProperties(ModelNode &templateNode, const ModelNode &styleNode);
    void adjustNodeIndex(ModelNode &node);
    void setupIdRenamingHash();
    ModelNode createReplacementNode(const ModelNode &styleNode, ModelNode &modelNode);
    void syncNodeProperties(ModelNode &outputNode, const ModelNode &inputNode, bool skipDuplicates = false);
    void syncNodeListProperties(ModelNode &outputNode, const ModelNode &inputNode, bool skipDuplicates = false);
    void syncId(ModelNode &outputNode, ModelNode &inputNode);
    void syncBindingProperties(ModelNode &outputNode, const ModelNode &inputNode);
    void syncAuxiliaryProperties(ModelNode &outputNode, const ModelNode &inputNode);
    void syncVariantProperties(ModelNode &outputNode, const ModelNode &inputNode);
    void parseTemplateOptions();

    AbstractView *m_templateView;
    AbstractView *m_styleView;
    QHash<QString, ReparentInfo> m_reparentInfoHash;
    QHash<QString, QString> m_idReplacementHash;

    struct Options {
        bool preserveTextAlignment;
        bool useStyleSheetPositions;
        Options()
            : preserveTextAlignment(false)
            , useStyleSheetPositions(false)
        {}
    };

    Options m_options;

};

}
#endif // STYLESHEETMERGER_H
