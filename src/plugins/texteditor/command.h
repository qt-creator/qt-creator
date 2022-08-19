// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QString>
#include <QStringList>

namespace TextEditor {

class TEXTEDITOR_EXPORT Command
{
public:
    enum Processing {
        FileProcessing,
        PipeProcessing
    };

    bool isValid() const;

    QString executable() const;
    void setExecutable(const QString &executable);

    QStringList options() const;
    void addOption(const QString &option);

    Processing processing() const;
    void setProcessing(const Processing &processing);

    bool pipeAddsNewline() const;
    void setPipeAddsNewline(bool pipeAddsNewline);

    bool returnsCRLF() const;
    void setReturnsCRLF(bool returnsCRLF);

private:
    QString m_executable;
    QStringList m_options;
    Processing m_processing = FileProcessing;
    bool m_pipeAddsNewline = false;
    bool m_returnsCRLF = false;
};

} // namespace TextEditor
