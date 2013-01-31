/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BASETEXTMARK_H
#define BASETEXTMARK_H

#include "texteditor_global.h"
#include "itexteditor.h"

#include <utils/fileutils.h>

#include <QWeakPointer>
#include <QHash>
#include <QSet>

QT_BEGIN_NAMESPACE
class QTextBlock;
class QPainter;
QT_END_NAMESPACE

namespace TextEditor {
namespace Internal {
class BaseTextMarkRegistry;
}

class ITextMarkable;

class TEXTEDITOR_EXPORT BaseTextMark : public TextEditor::ITextMark
{
    friend class Internal::BaseTextMarkRegistry;

public:
    BaseTextMark(const QString &fileName, int lineNumber);
    void init();
    virtual ~BaseTextMark();

    /// called if the filename of the document changed
    virtual void updateFileName(const QString &fileName);

    // access to internal data
    QString fileName() const { return m_fileName; }

private:
    QString m_fileName;
};

namespace Internal {
class BaseTextMarkRegistry : public QObject
{
    Q_OBJECT
public:
    BaseTextMarkRegistry(QObject *parent);

    void add(BaseTextMark *mark);
    bool remove(BaseTextMark *mark);
private slots:
    void editorOpened(Core::IEditor *editor);
    void documentRenamed(Core::IDocument *document, const QString &oldName, const QString &newName);
    void allDocumentsRenamed(const QString &oldName, const QString &newName);
private:
    QHash<Utils::FileName, QSet<BaseTextMark *> > m_marks;
};
}

} // namespace TextEditor

#endif // BASETEXTMARK_H
