// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "static_lib/hello_static.h"
#include "shared_lib/hello_shared.h"
#include "source_set/hello_set.h"
#include <iostream>

int main(int argc, char* argv[]) {
  std::cout << "Running App..." << std::endl;
  HelloStatic();
  HelloShared();
  HelloSet();

  if (argc > 1) {
    std::cout << "Arguments: ";
    for (int i = 1; i < argc; ++i) {
      std::cout << argv[i] << " ";
    }
    std::cout << std::endl;
  }
  return 0;
}
