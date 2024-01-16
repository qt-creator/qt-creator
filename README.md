# Qt Creator

Qt Creator is a cross-platform, integrated development environment (IDE) for
application developers to create applications for multiple desktop, embedded,
and mobile device platforms.

The Qt Creator Manual is available at:

https://doc.qt.io/qtcreator/index.html

For an overview of the Qt Creator IDE, see:

https://doc.qt.io/qtcreator/creator-overview.html

## Supported Platforms

The standalone binary packages support the following platforms:

* Windows 10 (64-bit) or later
* (K)Ubuntu Linux 22.04 (64-bit) or later
* macOS 11 or later

When you compile Qt Creator yourself, the Qt version that you build with
determines the supported platforms.

## Contributing

For instructions on how to set up the Qt Creator repository to contribute
patches back to Qt Creator, please check:

https://wiki.qt.io/Setting_up_Gerrit

See the following page for information about our coding standard:

https://doc.qt.io/qtcreator-extending/coding-style.html

## Compiling Qt Creator

Prerequisites:

* Qt 6.2 or later. The Qt version that you use to build Qt Creator defines the
  minimum platform versions that the result supports
  (Windows 10, RHEL/CentOS 8.4, Ubuntu 20.04, macOS 10.15 for Qt 6.2).
* Qt WebEngine module for QtWebEngine based help viewer
* On Windows:
    * MinGW with GCC 9 or Visual Studio 2019 or later
    * Python 3.5 or later (optional, needed for the python enabled debug helper)
    * Debugging Tools for Windows (optional, for MSVC debugging support with CDB)
* On Mac OS X: latest Xcode
* On Linux: GCC 9 or later
* LLVM/Clang 10 or later (optional, LLVM/Clang 14 is recommended.
  See [instructions](#getting-llvmclang-for-the-clang-code-model) on how to
  get LLVM.
  The ClangFormat plugin uses the LLVM C++ API.
  Since the LLVM C++ API provides no compatibility guarantee,
  if later versions don't compile we don't support that version.)
* CMake
* Ninja (recommended)

The used toolchain has to be compatible with the one Qt was compiled with.

### Getting Qt Creator from Git

The official mirror of the Qt Creator repository is located at
https://code.qt.io/cgit/qt-creator/qt-creator.git/. Run

    git clone https://code.qt.io/qt-creator/qt-creator.git

to clone the Qt Creator sources from there. This creates a checkout of the
Qt Creator sources in the `qt-creator/` directory of your current working
directory.

Qt Creator relies on some submodules, like
[litehtml](https://github.com/litehtml) for displaying documentation. Get these
submodules with

    cd qt-creator  # switch to the sources, if you just ran git clone
    git submodule update --init --recursive

Note the `--recursive` in this command, which fetches also submodules within
submodules, and is necessary to get all the sources.

The git history contains some coding style cleanup commits, which you might
want to exclude for example when running `git blame`. Do this by running

    git config blame.ignoreRevsFile .gitignore-blame

### Linux and macOS

These instructions assume that Ninja is installed and in the `PATH`, Qt Creator
sources are located at `/path/to/qtcreator_sources`, Qt is installed in
`/path/to/Qt`, and LLVM is installed in `/path/to/llvm`.

Note that if you install Qt via the online installer, the path to Qt must
include the version number and compiler ABI. The path to the online installer
content is not enough.

Note that `/path/to/Qt` doesn't imply the full path depth like:
`$USER/Qt/6.2.4/gcc_64/lib/cmake/Qt6`, but only `$USER/Qt/6.2.4/gcc_64`.

See [instructions](#getting-llvmclang-for-the-clang-code-model) on how to
get LLVM.

    mkdir qtcreator_build
    cd qtcreator_build

    cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja "-DCMAKE_PREFIX_PATH=/path/to/Qt;/path/to/llvm" /path/to/qtcreator_sources
    cmake --build .

### Windows

These instructions assume that Ninja is installed and in the `PATH`, Qt Creator
sources are located at `\path\to\qtcreator_sources`, Qt is installed in
`\path\to\Qt`, and LLVM is installed in `\path\to\llvm`.

Note that if you install Qt via the online installer, the path to Qt must
include the version number and compiler ABI. The path to the online installer
content is not enough.

Note that `\path\to\Qt` doesn't imply the full path depth like:
`c:\Qt\6.2.4\msvc2019_64\lib\cmake\Qt6`, but only `c:/Qt/6.2.4/msvc2019_64`.
The usage of slashes `/` is intentional, since CMake has issues with backslashes `\`
in `CMAKE_PREFX_PATH`, they are interpreted as escape codes.

See [instructions](#getting-llvmclang-for-the-clang-code-model) on how to
get LLVM.

Decide which compiler to use: MinGW or Microsoft Visual Studio.

MinGW is available via the Qt online installer, for other options see
<https://wiki.qt.io/MinGW>. Run the commands below in a shell prompt that has
`<path_to_mingw>\bin` in the `PATH`.

For Microsoft Visual C++ you can use the "Build Tools for Visual Studio". Also
install the "Debugging Tools for Windows" from the Windows SDK installer. We
strongly recommend using the 64-bit version and 64-bit compilers on 64-bit
systems. Open the `x64 Native Tools Command Prompt for VS <version>` from the
start menu items that were created for Visual Studio, and run the commands
below in it.

    md qtcreator_build
    cd qtcreator_build

    cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja "-DCMAKE_PREFIX_PATH=/path/to/Qt;/path/to/llvm" \path\to\qtcreator_sources
    cmake --build .

Qt Creator can be registered as a post-mortem debugger. This can be done in the
options page or by running the tool qtcdebugger with administrative privileges
passing the command line options -register/unregister, respectively.
Alternatively, the required registry entries

    HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\AeDebug
    HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Windows NT\CurrentVersion\AeDebug

can be modified using the registry editor regedt32 to contain

    qtcreator_build\bin\qtcdebugger %ld %ld

When using a self-built version of Qt Creator as post-mortem debugger, it needs
to be able to find all dependent Qt-libraries and plugins when being launched
by the system. The easiest way to do this is to create a self-contained Qt
Creator by installing it and installing its dependencies. See "Options" below
for details.

Note that unlike on Unix, you cannot overwrite executables that are running.
Thus, if you want to work on Qt Creator using Qt Creator, you need a separate
installation of it. We recommend using a separate, release-built version of Qt
Creator to work on a debug-built version of Qt Creator.

Alternatively, take the following template of `CMakeUserPresets.json` for
reference. Write your own configurePreset inheriting `cmake-plugin-minimal` in
`CMakeUserPresets.json` to build with IDEs (such as QtCreator, VSCode,
CLion...etc) locally:

```json
{
  "version": 4,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 23,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "custom",
      "displayName": "custom",
      "description": "custom",
      "inherits": "cmake-plugin-minimal",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "toolset": {
        "value": "v142,host=x64",
        "strategy": "external"
      },
      "architecture": {
        "value": "x64",
        "strategy": "external"
      },
      "cacheVariables": {
        "CMAKE_CXX_COMPILER": "cl.exe",
        "CMAKE_C_COMPILER": "cl.exe",
        "CMAKE_PREFIX_PATH": "c:/Qt/6.2.4/msvc2019_64"
      }
    }
  ]
}
```

### Options

If you do not have Ninja installed and in the `PATH`, remove `-G Ninja` from
the first `cmake` call. If you want to build in release mode, change the build
type to `-DCMAKE_BUILD_TYPE=Release`. You can also build with release
optimizations but debug information with `-DCMAKE_BUILD_TYPE=RelWithDebInfo`.

You can find more options in the generated CMakeCache.txt file. For instance,
building of Qbs together with Qt Creator can be enabled with `-DBUILD_QBS=ON`.

Installation is not needed. You can run Qt Creator directly from the build
directory. On Windows, make sure that your `PATH` environment variable points to
all required DLLs, like Qt and LLVM. On Linux and macOS, the build already
contains the necessary `RPATH`s for the dependencies.

If you want to install Qt Creator anyway, that is however possible using

    cmake --install . --prefix /path/to/qtcreator_install

To create a self-contained Qt Creator installation, including all dependencies
like Qt and LLVM, additionally run

    cmake --install . --prefix /path/to/qtcreator_install --component Dependencies

To install development files like headers, CMake files, and `.lib` files on
Windows, run

    cmake --install . --prefix /path/to/qtcreator_install --component Devel

If you used the `RelWithDebInfo` configuration and want debug information to be
available to the installed Qt Creator, run

    cmake --install . --prefix /path/to/qtcreator_install --component DebugInfo

### Perf Profiler Support

Support for the [perf](https://perf.wiki.kernel.org/index.php/Main_Page) profiler
requires the `perfparser` tool that is part of the Qt Creator source package, and also
part of the Qt Creator Git repository in form of a submodule in `src/tools/perfparser`.

Compilation of `perfparser` requires ELF and DWARF development packages.
You can either download and extract a prebuilt package from
https://download.qt.io/development_releases/prebuilt/elfutils/ and add the
directory to the `CMAKE_PREFIX_PATH` when configuring Qt Creator,
or install the `libdw-dev` package on Debian-style Linux systems.

You can also point Qt Creator to a separate installation of `perfparser` by
setting the `PERFPROFILER_PARSER_FILEPATH` environment variable to the full
path to the executable.

## Getting LLVM/Clang for the Clang Code Model

The Clang code model uses `Clangd` and the ClangFormat plugin depends on the
LLVM/Clang libraries. The currently recommended LLVM/Clang version is 14.0.

### Prebuilt LLVM/Clang packages

Prebuilt packages of LLVM/Clang can be downloaded from
https://download.qt.io/development_releases/prebuilt/libclang/

This should be your preferred option because you will use the version that is
shipped together with Qt Creator (with backported/additional patches). In
addition, MinGW packages for Windows are faster due to profile-guided
optimization. If the prebuilt packages do not match your configuration, you
need to build LLVM/Clang manually.

If you use the MSVC compiler to build Qt Creator the suggested way is:
    1. Download both MSVC and MinGW packages of libclang.
    2. Use the MSVC version of libclang during the Qt Creator build.
    3. Prepend PATH variable used for the run time with the location of MinGW version of libclang.dll.
    4. Launch Qt Creator.

### Building LLVM/Clang manually

You need to install CMake in order to build LLVM/Clang.

Build LLVM/Clang by roughly following the instructions at
http://llvm.org/docs/GettingStarted.html#git-mirror:

   1. Clone LLVM/Clang and checkout a suitable branch

          git clone -b release_130-based --recursive https://code.qt.io/clang/llvm-project.git

   2. Build and install LLVM/Clang

          mkdir build
          cd build

      For Linux/macOS:

          cmake \
            -D CMAKE_BUILD_TYPE=Release \
            -D LLVM_ENABLE_RTTI=ON \
            -D LLVM_ENABLE_PROJECTS="clang;clang-tools-extra" \
            -D CMAKE_INSTALL_PREFIX=<installation location> \
            ../llvm-project/llvm
          cmake --build . --target install

      For Windows:

          cmake ^
            -G Ninja ^
            -D CMAKE_BUILD_TYPE=Release ^
            -D LLVM_ENABLE_RTTI=ON ^
            -D LLVM_ENABLE_PROJECTS="clang;clang-tools-extra" ^
            -D CMAKE_INSTALL_PREFIX=<installation location> ^
            ..\llvm-project\llvm
          cmake --build . --target install

### Clang-Format

The ClangFormat plugin depends on the additional patch

    https://code.qt.io/cgit/clang/llvm-project.git/commit/?h=release_130-based&id=42879d1f355fde391ef46b96a659afeb4ad7814a

While the plugin builds without it, it might not be fully functional.

Note that the plugin is disabled by default.

# Licenses and Attributions

Qt Creator is available under commercial licenses from The Qt Company,
and under the GNU General Public License version 3,
annotated with The Qt Company GPL Exception 1.0.
See [LICENSE.GPL-EXCEPT](LICENSE.GPL-EXCEPT) for the details.

Qt Creator furthermore includes the following third-party components,
we thank the authors who made this possible:

### YAML Parser yaml-cpp (MIT License)

  https://github.com/jbeder/yaml-cpp

  QtCreator/src/libs/3rdparty/yaml-cpp

  Copyright (c) 2008-2015 Jesse Beder.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

### KSyntaxHighlighting

  Syntax highlighting engine for Kate syntax definitions

  This is a stand-alone implementation of the Kate syntax highlighting
  engine. It's meant as a building block for text editors as well as
  for simple highlighted text rendering (e.g. as HTML), supporting both
  integration with a custom editor as well as a ready-to-use
  QSyntaxHighlighter sub-class.

  Distributed under the:

  MIT License

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to
  permit persons to whom the Software is furnished to do so, subject to
  the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

  The source code of KSyntaxHighlighting can be found here:
      https://cgit.kde.org/syntax-highlighting.git
      QtCreator/src/libs/3rdparty/syntax-highlighting
      https://code.qt.io/cgit/qt-creator/qt-creator.git/tree/src/libs/3rdparty/syntax-highlighting

### Clazy

  https://github.com/KDE/clazy

  Copyright (C) 2015-2018 Clazy Team

  Distributed under GNU LIBRARY GENERAL PUBLIC LICENSE Version 2 (LGPL2).

  Integrated with patches from
  https://code.qt.io/cgit/clang/clazy.git/.

### LLVM/Clang

  https://github.com/llvm/llvm-project.git

  Copyright (C) 2003-2019 LLVM Team

  Distributed under the Apache 2.0 License with LLVM exceptions,
  see https://github.com/llvm/llvm-project/blob/main/clang/LICENSE.TXT

  With backported/additional patches from https://code.qt.io/cgit/clang/llvm-project.git

### std::span implementation for C++11 and later

  A single-header implementation of C++20's std::span, conforming to the C++20
  committee draft. It is compatible with C++11, but will use newer language
  features if they are available.

  https://github.com/martinmoene/span-lite

  QtCreator/src/libs/3rdparty/span

  Copyright 2018-2021 Martin Moene

  Distributed under the Boost Software License, Version 1.0.
  (See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)

### Open Source front-end for C++ (license MIT), enhanced for use in Qt Creator

  Roberto Raggi <roberto.raggi@gmail.com>

  QtCreator/src/libs/3rdparty/cplusplus

  Copyright 2005 Roberto Raggi <roberto@kdevelop.org>

  Permission to use, copy, modify, distribute, and sell this software and its
  documentation for any purpose is hereby granted without fee, provided that
  the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  KDEVELOP TEAM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

### Open Source tool for generating C++ code that classifies keywords (license MIT)

  Roberto Raggi <roberto.raggi@gmail.com>

  QtCreator/src/tools/3rdparty/cplusplus-keywordgen

  Copyright (c) 2007 Roberto Raggi <roberto.raggi@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to
  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
  the Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

### SQLite (version 3.8.10.2)

SQLite is a C-language library that implements a small, fast, self-contained,
high-reliability, full-featured, SQL database engine.

SQLite (https://www.sqlite.org) is in the Public Domain.

### ClassView and ImageViewer plugins

  Copyright (C) 2016 The Qt Company Ltd.

  All rights reserved.
  Copyright (C) 2016 Denis Mingulov.

  Contact: http://www.qt.io

  This file is part of Qt Creator.

  You may use this file under the terms of the BSD license as follows:

  "Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of The Qt Company Ltd and its Subsidiary(-ies) nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."

### Source Code Pro font

  Copyright 2010, 2012 Adobe Systems Incorporated (http://www.adobe.com/),
  with Reserved Font Name 'Source'. All Rights Reserved. Source is a
  trademark of Adobe Systems Incorporated in the United States
  and/or other countries.

  This Font Software is licensed under the SIL Open Font License, Version 1.1.

  The font and license files can be found in QtCreator/src/libs/3rdparty/fonts.

### JSON Library by Niels Lohmann

  Used by the Chrome Trace Format Visualizer plugin instead of QJson
  because of QJson's current hard limit of 128 Mb object size and
  trace files often being much larger.

  The sources can be found in `QtCreator/src/libs/3rdparty/json`.

  The class is licensed under the MIT License:

  Copyright © 2013-2019 Niels Lohmann

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the “Software”), to
  deal in the Software without restriction, including without limitation the
  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is furnished
  to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

  The class contains the UTF-8 Decoder from Bjoern Hoehrmann which is
  licensed under the MIT License (see above). Copyright © 2008-2009 Björn
  Hoehrmann bjoern@hoehrmann.de

  The class contains a slightly modified version of the Grisu2 algorithm
  from Florian Loitsch which is licensed under the MIT License (see above).
  Copyright © 2009 Florian Loitsch

### litehtml

  The litehtml HTML/CSS rendering engine is used as a help viewer backend
  to display help files.

  The sources can be found in:
    * QtCreator/src/plugins/help/qlitehtml
    * https://github.com/litehtml

  Copyright (c) 2013, Yuri Kobets (tordex)

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
   * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   * Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   * Neither the name of the <organization> nor the
   names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### gumbo

  The litehtml HTML/CSS rendering engine uses the gumbo parser.

  Copyright 2010, 2011 Google

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

### gumbo/utf8.c

  The litehtml HTML/CSS rendering engine uses gumbo/utf8.c parser.

  Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do  so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

### SourceCodePro fonts

  Qt Creator ships with the following fonts licensed under OFL-1.1:

   * SourceCodePro-Regular.ttf
   * SourceCodePro-It.ttf
   * SourceCodePro-Bold.ttf

  SIL OPEN FONT LICENSE

  Version 1.1 - 26 February 2007

  PREAMBLE
  The goals of the Open Font License (OFL) are to stimulate worldwide
  development of collaborative font projects, to support the font creation
  efforts of academic and linguistic communities, and to provide a free and
  open framework in which fonts may be shared and improved in partnership
  with others.

  The OFL allows the licensed fonts to be used, studied, modified and
  redistributed freely as long as they are not sold by themselves. The
  fonts, including any derivative works, can be bundled, embedded,
  redistributed and/or sold with any software provided that any reserved
  names are not used by derivative works. The fonts and derivatives,
  however, cannot be released under any other type of license. The
  requirement for fonts to remain under this license does not apply
  to any document created using the fonts or their derivatives.

  DEFINITIONS
  "Font Software" refers to the set of files released by the Copyright
  Holder(s) under this license and clearly marked as such. This may
  include source files, build scripts and documentation.

  "Reserved Font Name" refers to any names specified as such after the
  copyright statement(s).

  "Original Version" refers to the collection of Font Software components as
  distributed by the Copyright Holder(s).

  "Modified Version" refers to any derivative made by adding to, deleting,
  or substituting - in part or in whole - any of the components of the
  Original Version, by changing formats or by porting the Font Software to a
  new environment.

  "Author" refers to any designer, engineer, programmer, technical
  writer or other person who contributed to the Font Software.

  PERMISSION & CONDITIONS
  Permission is hereby granted, free of charge, to any person obtaining
  a copy of the Font Software, to use, study, copy, merge, embed, modify,
  redistribute, and sell modified and unmodified copies of the Font
  Software, subject to the following conditions:

  1) Neither the Font Software nor any of its individual components,
  in Original or Modified Versions, may be sold by itself.

  2) Original or Modified Versions of the Font Software may be bundled,
  redistributed and/or sold with any software, provided that each copy
  contains the above copyright notice and this license. These can be
  included either as stand-alone text files, human-readable headers or
  in the appropriate machine-readable metadata fields within text or
  binary files as long as those fields can be easily viewed by the user.

  3) No Modified Version of the Font Software may use the Reserved Font
  Name(s) unless explicit written permission is granted by the corresponding
  Copyright Holder. This restriction only applies to the primary font name as
  presented to the users.

  4) The name(s) of the Copyright Holder(s) or the Author(s) of the Font
  Software shall not be used to promote, endorse or advertise any
  Modified Version, except to acknowledge the contribution(s) of the
  Copyright Holder(s) and the Author(s) or with their explicit written
  permission.

  5) The Font Software, modified or unmodified, in part or in whole,
  must be distributed entirely under this license, and must not be
  distributed under any other license. The requirement for fonts to
  remain under this license does not apply to any document created
  using the Font Software.

  TERMINATION
  This license becomes null and void if any of the above conditions are
  not met.

  DISCLAIMER
  THE FONT SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
  OF COPYRIGHT, PATENT, TRADEMARK, OR OTHER RIGHT. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
  INCLUDING ANY GENERAL, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL
  DAMAGES, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF THE USE OR INABILITY TO USE THE FONT SOFTWARE OR FROM
  OTHER DEALINGS IN THE FONT SOFTWARE.

### Qbs

  Qt Creator installations deliver Qbs. Its licensing and third party
  attributions are listed in Qbs Manual at
  https://doc.qt.io/qbs/attributions.html

### conan.cmake

  CMake script used by Qt Creator's auto setup of package manager dependencies.

  The sources can be found in:
    * QtCreator/src/share/3rdparty/package-manager/conan.cmake
    * https://github.com/conan-io/cmake-conan

  The MIT License (MIT)

  Copyright (c) 2018 JFrog

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

### TartanLlama/expected

  Implementation of std::expected compatible with C++11/C++14/C++17.

  https://github.com/TartanLlama/expected

  To the extent possible under law, the author(s) have dedicated all
  copyright and related and neighboring rights to this software to the
  public domain worldwide. This software is distributed without any warranty.

  http://creativecommons.org/publicdomain/zero/1.0/

### WinPty

  Implementation of a pseudo terminal for Windows.

  https://github.com/rprichard/winpty

  The MIT License (MIT)

  Copyright (c) 2011-2016 Ryan Prichard

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to
  deal in the Software without restriction, including without limitation the
  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
  sell copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.


### ptyqt

  Pty-Qt is small library for access to console applications by pseudo-terminal interface on Mac,
  Linux and Windows. On Mac and Linux it uses standard PseudoTerminal API and on Windows it uses
  WinPty(prefer) or ConPty.

  https://github.com/kafeg/ptyqt

  MIT License

  Copyright (c) 2019 Vitaly Petrov, v31337@gmail.com

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

### libvterm

  An abstract C99 library which implements a VT220 or xterm-like terminal emulator.
  It doesn't use any particular graphics toolkit or output system, instead it invokes callback
  function pointers that its embedding program should provide it to draw on its behalf.
  It avoids calling malloc() during normal running state, allowing it to be used in embedded kernel
  situations.

  https://www.leonerd.org.uk/code/libvterm/

  The MIT License

  Copyright (c) 2008 Paul Evans <leonerd@leonerd.org.uk>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

### terminal/shellintegrations

  The Terminal plugin uses scripts to integrate with the shell. The scripts are
  located in the Qt Creator source tree in src/plugins/terminal/shellintegrations.

  https://github.com/microsoft/vscode/tree/main/src/vs/workbench/contrib/terminal/browser/media

  MIT License

  Copyright (c) 2015 - present Microsoft Corporation

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

### terminal/shellintegrations/clink

  The Terminal plugin uses a lua script to integrate with the cmd shell when using clink.

  https://github.com/chrisant996/clink-gizmos

  MIT License

  Copyright (c) 2023 Chris Antos

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

### cmake

  The CMake project manager uses the CMake lexer code for parsing CMake files

  https://gitlab.kitware.com/cmake/cmake.git

  CMake - Cross Platform Makefile Generator
  Copyright 2000-2023 Kitware, Inc. and Contributors
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

  * Neither the name of Kitware, Inc. nor the names of Contributors
    may be used to endorse or promote products derived from this
    software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### RSTParser

  RSTParser is an open-source C++ library for parsing reStructuredText

  https://github.com/vitaut-archive/rstparser

  License
  -------

  Copyright (c) 2013, Victor Zverovich

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
