// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include <utils/result.h>

#include <QByteArray>

class QIODevice;

namespace CommonTraceFormat {

// True if the device starts with the CTF 1.8 metadata packet magic (0x75D11D57).
// Does NOT advance the device position.
CMNTRCEFMT_EXPORT bool isCTF1PacketizedMetadata(QIODevice *dev);

// True if the device looks like a CTF 1.8 plaintext TSDL metadata file (the
// non-packetized form, e.g. as emitted by perf): the first non-whitespace byte
// is not the CTF 2 record-separator (0x1E). The caller MUST rule out the
// packetized form with isCTF1PacketizedMetadata() first, since that magic also begins
// with a non-0x1E byte. Does NOT advance the device position.
CMNTRCEFMT_EXPORT bool isCTF1PlaintextMetadata(QIODevice *dev);

// Extract and concatenate the TSDL text from all CTF 1.8 metadata packets.
// The device must be positioned at the start.
CMNTRCEFMT_EXPORT Utils::Result<QByteArray> readCTF1TsdlText(QIODevice *dev);

} // namespace CommonTraceFormat
