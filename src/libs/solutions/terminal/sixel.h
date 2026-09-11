// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "terminal_global.h"

#include <QByteArrayView>
#include <QImage>

#include <vector>

namespace TerminalSolution {

// Decodes the data string of a sixel DCS, DEC's bitmap graphics format:
// DCS P1 ; P2 ; P3 q <data> ST. The data arrives from the parser in fragments,
// so it is fed in as it comes and the image read once the string ends.
class TERMINAL_EXPORT SixelDecoder
{
public:
    // Largest image the decoder produces. Data beyond it is dropped.
    static constexpr int maxDimension = 8192;
    static constexpr int maxPixels = 8 * 1024 * 1024;

    static constexpr int colorRegisterCount = 256;

    void start();
    void addData(QByteArrayView data);
    // The image decoded since the last start(), and a fresh start for the next one.
    QImage takeImage();

    bool isStarted() const { return m_started; }

private:
    void handleByte(char c);
    void endCommand();
    void paint(int bits, int repeat);
    bool grow(int width, int height);
    QRgb *scanLine(int y) { return m_pixels.data() + size_t(y) * size_t(m_width); }

    bool m_started = false;
    bool m_full = false;

    std::vector<QRgb> m_pixels;
    int m_width = 0;
    int m_height = 0;
    int m_usedWidth = 0;
    int m_usedHeight = 0;

    std::vector<QRgb> m_palette;
    QRgb m_color = qRgb(0, 0, 0);

    int m_x = 0;
    int m_bandTop = 0;
    int m_repeat = 1;

    char m_command = 0;
    std::vector<int> m_parameters;
    bool m_inParameter = false;
};

} // namespace TerminalSolution
