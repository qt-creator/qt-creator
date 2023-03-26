// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#define QARK_FRIEND_ACCESS \
    template<class Archive, class T> \
    friend class qark::Access;
