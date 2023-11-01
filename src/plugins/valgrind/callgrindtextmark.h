// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textmark.h>

#include <QPersistentModelIndex>

namespace Valgrind::Callgrind { class Function; }

namespace Valgrind::Internal {

class CallgrindTextMark : public TextEditor::TextMark
{
public:
    /**
     * This creates a callgrind text mark for a specific Function
     *
     * \param index DataModel model index
     * \note The index parameter must refer to one of the DataModel cost columns
     */
    explicit CallgrindTextMark(const QPersistentModelIndex &index,
                               const Utils::FilePath &fileName, int lineNumber);

    const Valgrind::Callgrind::Function *function() const;

private:
    bool addToolTipContent(QLayout *target) const;
    qreal costs() const;

    QPersistentModelIndex m_modelIndex;
};

} // namespace Valgrind::Internal
