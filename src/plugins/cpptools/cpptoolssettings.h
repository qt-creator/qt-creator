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

#include "cpptools_global.h"

#include <QObject>

namespace TextEditor {
class CommentsSettings;
}

namespace CppTools
{
class CppCodeStylePreferences;

namespace Internal
{
class CppToolsSettingsPrivate;
}

/**
 * This class provides a central place for cpp tools settings.
 */
class CPPTOOLS_EXPORT CppToolsSettings : public QObject
{
    Q_OBJECT

public:
    CppToolsSettings();
    ~CppToolsSettings() override;

    static CppToolsSettings *instance();

    CppCodeStylePreferences *cppCodeStyle() const;

    const TextEditor::CommentsSettings &commentsSettings() const;
    void setCommentsSettings(const TextEditor::CommentsSettings &commentsSettings);

    bool sortedEditorDocumentOutline() const;
    void setSortedEditorDocumentOutline(bool sorted);

    bool showHeaderErrorInfoBar() const;
    void setShowHeaderErrorInfoBar(bool show);

    bool showNoProjectInfoBar() const;
    void setShowNoProjectInfoBar(bool show);

signals:
    void editorDocumentOutlineSortingChanged(bool isSorted);
    void showHeaderErrorInfoBarChanged(bool isShown);
    void showNoProjectInfoBarChanged(bool isShown);

private:
    Internal::CppToolsSettingsPrivate *d;

    static CppToolsSettings *m_instance;
};

} // namespace CppTools
