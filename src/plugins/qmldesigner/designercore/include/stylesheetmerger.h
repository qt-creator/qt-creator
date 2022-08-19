// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#ifndef STYLESHEETMERGER_H
#define STYLESHEETMERGER_H

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


class StylesheetMerger {
public:
    StylesheetMerger(AbstractView*, AbstractView*);
    void merge();
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
