// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#pragma once

#include <QXmlStreamReader>

namespace ADS {

/**
 * Extends QXmlStreamReader with file version information
 */
class DockingStateReader : public QXmlStreamReader
{
private:
    int m_fileVersion;

public:
    using QXmlStreamReader::QXmlStreamReader;

    /**
     * Set the file version for this state reader
     */
    void setFileVersion(int fileVersion);

    /**
     * Returns the file version set via setFileVersion
     */
    int fileVersion() const;
};

} // namespace ADS
