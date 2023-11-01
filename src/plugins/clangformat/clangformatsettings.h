// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace ClangFormat {

class ClangFormatSettings
{
public:
    static ClangFormatSettings &instance();

    ClangFormatSettings();
    void write() const;

    void setOverrideDefaultFile(bool enable);
    bool overrideDefaultFile() const;

    enum Mode {
        Indenting = 0,
        Formatting,
        Disable
    };

    void setMode(Mode mode);
    Mode mode() const;

    void setFormatWhileTyping(bool enable);
    bool formatWhileTyping() const;

    void setFormatOnSave(bool enable);
    bool formatOnSave() const;

    void setFileSizeThreshold(int fileSizeInKb);
    int fileSizeThreshold() const;

private:
    Mode m_mode;
    bool m_overrideDefaultFile = false;
    bool m_formatWhileTyping = false;
    bool m_formatOnSave = false;
    int m_fileSizeThreshold = 200;
};

} // namespace ClangFormat
