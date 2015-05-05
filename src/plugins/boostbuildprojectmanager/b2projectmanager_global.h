//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#ifndef BBPROJECTMANAGER_GLOBAL_HPP
#define BBPROJECTMANAGER_GLOBAL_HPP

#include <QtGlobal>

#if defined(BOOSTBUILDPROJECTMANAGER_LIBRARY)
#  define BOOSTBUILDPROJECTMANAGER_EXPORT Q_DECL_EXPORT
#else
#  define BOOSTBUILDPROJECTMANAGER_EXPORT Q_DECL_IMPORT
#endif

#endif // BBPROJECTMANAGER_GLOBAL_HPP
