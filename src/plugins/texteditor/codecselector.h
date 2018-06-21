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

#include <QDialog>
#include <QLabel>
#include <QDialogButtonBox>
#include <QListWidget>

namespace Utils { class ListWidget; }

namespace TextEditor {

class TextDocument;

namespace Internal {

class CodecSelector : public QDialog
{
    Q_OBJECT

public:

    CodecSelector(QWidget *parent, TextDocument *doc);
    ~CodecSelector() override;

    QTextCodec *selectedCodec() const;

    // Enumeration returned from QDialog::exec()
    enum Result {
        Cancel, Reload, Save
    };

private:
    void updateButtons();
    void buttonClicked(QAbstractButton *button);

    bool m_hasDecodingError;
    bool m_isModified;
    QLabel *m_label;
    Utils::ListWidget *m_listWidget;
    QDialogButtonBox *m_dialogButtonBox;
    QAbstractButton *m_reloadButton;
    QAbstractButton *m_saveButton;
};

} // namespace Internal
} // namespace TextEditor
