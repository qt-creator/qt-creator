/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
