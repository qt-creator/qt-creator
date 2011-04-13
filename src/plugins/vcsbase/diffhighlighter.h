/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef DIFFHIGHLIGHTER_H
#define DIFFHIGHLIGHTER_H

#include "vcsbase_global.h"

#include <texteditor/syntaxhighlighter.h>

QT_BEGIN_NAMESPACE
class QRegExp;
class QTextCharFormat;
QT_END_NAMESPACE

namespace Core {
    class ICore;
}
namespace TextEditor {
    class FontSettingsPage;
}

namespace VCSBase {

namespace Internal {
class DiffHighlighterPrivate;
} // namespace Internal

/* A highlighter for diffs. Parametrizable by the file indicator,
 *  which is for example '^====' in case of p4:
 * \code
   ==== //depot/research/main/qdynamicmainwindow3/qdynamicdockwidgetlayout_p.h#34 (text) ====
 * \endcode
 * Or  '--- a/|'+++ b/' in case of git:
 * \code
   diff --git a/src/plugins/plugins.pro b/src/plugins/plugins.pro
   index 9401ee7..ef35c3b 100644
   --- a/src/plugins/plugins.pro
   +++ b/src/plugins/plugins.pro
   @@ -10,6 +10,7 @@ SUBDIRS   = plugin_coreplugin \
 * \endcode
 * */

class VCSBASE_EXPORT DiffHighlighter : public TextEditor::SyntaxHighlighter
{
    Q_OBJECT
public:
    explicit DiffHighlighter(const QRegExp &filePattern,
                             QTextDocument *document = 0);
    virtual ~DiffHighlighter();

    virtual void highlightBlock(const QString &text);

    // Set formats from a sequence of type QTextCharFormat
    void setFormats(const QVector<QTextCharFormat> &s);

    QRegExp filePattern() const;

private:
    Internal::DiffHighlighterPrivate *m_d;
};

} // namespace VCSBase

#endif // DIFFHIGHLIGHTER_H
