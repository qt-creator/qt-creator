// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "dockingstatereader.h"

namespace ADS {

void DockingStateReader::setFileVersion(int fileVersion)
{
    m_fileVersion = fileVersion;
}

int DockingStateReader::fileVersion() const
{
    return m_fileVersion;
}

} // namespace ADS
