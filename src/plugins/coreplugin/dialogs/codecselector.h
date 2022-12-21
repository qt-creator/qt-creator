// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <QDialog>
#include <QLabel>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QTextCodec>

namespace Utils { class ListWidget; }
namespace Core { class BaseTextDocument; }

namespace Core {

class CORE_EXPORT CodecSelector : public QDialog
{
    Q_OBJECT

public:

    CodecSelector(QWidget *parent, Core::BaseTextDocument *doc);
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

} // namespace Core
