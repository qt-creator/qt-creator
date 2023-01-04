// Copyright (C) 2020 Uwe Kindler
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

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
