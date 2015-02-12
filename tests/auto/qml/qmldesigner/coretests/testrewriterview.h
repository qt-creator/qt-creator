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

#ifndef TESTREWRITERVIEW_H
#define TESTREWRITERVIEW_H

#include <rewriterview.h>
#include <model/modeltotextmerger.h>

namespace QmlDesigner {

namespace Internal {

class TestModelToTextMerger : public ModelToTextMerger
{
public:
    bool isNodeScheduledForRemoval(const ModelNode &node) const;
    bool isNodeScheduledForAddition(const ModelNode &node) const;
    VariantProperty findAddedVariantProperty(const VariantProperty &property) const;
};

} // Internal

class TestRewriterView : public RewriterView
{

Q_OBJECT

public:
    TestRewriterView(QObject *parent = 0,
                     DifferenceHandling differenceHandling = RewriterView::Validate);

    Internal::TestModelToTextMerger *modelToTextMerger() const;
    Internal::TextToModelMerger *textToModelMerger() const;

    bool isModificationGroupActive() const;
    void setModificationGroupActive(bool active);
    void applyModificationGroupChanges();
};

} // QmlDesigner

#endif // TESTREWRITERVIEW_H
