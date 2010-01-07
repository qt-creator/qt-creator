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

#ifndef ANCHORMANIPULATOR_H
#define ANCHORMANIPULATOR_H

#include <qmlanchors.h>

namespace QmlDesigner {

class FormEditorItem;
class FormEditorView;

class AnchorManipulator
{
public:
    AnchorManipulator(FormEditorView *view);
    ~AnchorManipulator();
    void begin(FormEditorItem *beginItem, AnchorLine::Type anchorLine);
    void addAnchor(FormEditorItem *endItem, AnchorLine::Type anchorLine);
    void removeAnchor();

    void clear();

    bool isActive() const;

    bool beginAnchorLineIsHorizontal() const;
    bool beginAnchorLineIsVertical() const;

    AnchorLine::Type beginAnchorLine() const;

    FormEditorItem *beginFormEditorItem() const;

private: // fucntions
    void setMargin(FormEditorItem *endItem, AnchorLine::Type endAnchorLine);

private: // variables
    FormEditorItem *m_beginFormEditorItem;
    AnchorLine::Type m_beginAnchorLine;
    QWeakPointer<FormEditorView> m_view;
};

} // namespace QmlDesigner

#endif // ANCHORMANIPULATOR_H
