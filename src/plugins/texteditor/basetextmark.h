/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef BASETEXTMARK_H
#define BASETEXTMARK_H

#include "texteditor_global.h"
#include "itexteditor.h"

#include <QtCore/QPointer>

QT_BEGIN_NAMESPACE
class QTextBlock;
class QPainter;
QT_END_NAMESPACE

namespace TextEditor {

class ITextMarkable;

class TEXTEDITOR_EXPORT BaseTextMark : public TextEditor::ITextMark
{
    Q_OBJECT

public:
    BaseTextMark();
    virtual ~BaseTextMark();

    // our location in the "owning" edtitor
    virtual void setLocation(const QString &fileName, int lineNumber);

    // call this if the icon has changed.
    void updateMarker();

    // access to internal data
    QString fileName() const { return m_fileName; }
    int lineNumber() const { return m_line; }

    void moveMark(const QString &filename, int line);

private slots:
    void init();
    void editorOpened(Core::IEditor *editor);
    void documentReloaded();

private:
    void removeInternalMark();

    QPointer<ITextMarkable> m_markableInterface;
    QString m_fileName;
    int m_line;
    bool m_init;
};

} // namespace TextEditor

#endif // BASETEXTMARK_H
