// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Utils { class FilePath; }

namespace Nim::Suggest {

class NimSuggest;

NimSuggest *getFromCache(const Utils::FilePath &filePath);

} // Nim::Suggest
