/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

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
    QString defaultToolTip(Core::Id category) const;
    void setDefaultToolTip(Core::Id category, const QString &toolTip);
private:
    void editorOpened(Core::IEditor *editor);
    void documentRenamed(Core::IDocument *document, const QString &oldName, const QString &newName);
    void allDocumentsRenamed(const QString &oldName, const QString &newName);

    QHash<Utils::FileName, QSet<TextMark *> > m_marks;
    QHash<Core::Id, Utils::Theme::Color> m_colors;
    QHash<Core::Id, QString> m_defaultToolTips;
};

} // namespace Internal
} // namespace TextEditor
