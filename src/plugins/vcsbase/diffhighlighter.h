/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef DIFFHIGHLIGHTER_H
#define DIFFHIGHLIGHTER_H

#include "vcsbase_global.h"

#include <QtGui/QSyntaxHighlighter>
#include <QtGui/QTextCharFormat>
#include <QtCore/QVector>

QT_BEGIN_NAMESPACE
class QRegExp;
QT_END_NAMESPACE

namespace Core {
    class ICore;
}
namespace TextEditor {
    class FontSettingsPage;
}

namespace VCSBase {

struct DiffHighlighterPrivate;

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

class VCSBASE_EXPORT DiffHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit DiffHighlighter(const QRegExp &filePattern,
                             QTextDocument *document = 0);
    virtual ~DiffHighlighter();

    virtual void highlightBlock(const QString &text);

    // Set formats from a sequence of type QTextCharFormat
    void setFormats(const QVector<QTextCharFormat> &s);

private:
    DiffHighlighterPrivate *m_d;
};

} // namespace VCSBase

#endif // DIFFHIGHLIGHTER_H
