/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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

private: // functions
    void setMargin(FormEditorItem *endItem, AnchorLine::Type endAnchorLine);

private: // variables
    FormEditorItem *m_beginFormEditorItem;
    AnchorLine::Type m_beginAnchorLine;
    QWeakPointer<FormEditorView> m_view;
};

} // namespace QmlDesigner

#endif // ANCHORMANIPULATOR_H
