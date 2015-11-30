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

#ifndef BASETEXTMARKREGISTRY_H
#define BASETEXTMARKREGISTRY_H

#include <utils/fileutils.h>

#include <QColor>
#include <QHash>
#include <QObject>
#include <QSet>

#include <coreplugin/id.h>
#include <utils/theme/theme.h>

namespace Core {
class IEditor;
class IDocument;
}

namespace TextEditor {
class TextMark;
namespace Internal {

class TextMarkRegistry : public QObject
{
    Q_OBJECT
public:
    TextMarkRegistry(QObject *parent);

    void add(TextMark *mark);
    bool remove(TextMark *mark);
    Utils::Theme::Color categoryColor(Core::Id category);
    bool categoryHasColor(Core::Id category);
    void setCategoryColor(Core::Id category, Utils::Theme::Color color);
private slots:
    void editorOpened(Core::IEditor *editor);
    void documentRenamed(Core::IDocument *document, const QString &oldName, const QString &newName);
    void allDocumentsRenamed(const QString &oldName, const QString &newName);
private:
    QHash<Utils::FileName, QSet<TextMark *> > m_marks;
    QHash<Core::Id, Utils::Theme::Color> m_colors;
};

} // namespace Internal
} // namespace TextEditor

#endif // BASETEXTMARKREGISTRY_H
