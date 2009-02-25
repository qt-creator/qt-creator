/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef QTEXTENDED_INTEGRATION
#define QTEXTENDED_INTEGRATION

// Enable support for QBuild style project parsing
//#define QTEXTENDED_QBUILD_SUPPORT

/*
This file is intended to be the 'communication channel' for Qt-Extended (aka Qtopia) integration issues.

*** The first step in the integration is to add support for QBuild to the IDE ***

QBuild is the build system used in Qt-Extended 4.4 and onwards. QBuild is a replacement for qmake+make.
The reasons for the replacement go a bit beyond this page, but in short:
- there's only one qbuild instance running v.s. MANY qmake instances.
- implements proper management of dependencies between files. Regardless of where you are in the source tree ... a 'make' will always result in all dependencies (that have changed) being rebuild in the correct sequence. This is something that was a major issue with Qt-Extended development in the past.
- solves many Qt-Extended build/deployment issues
- separates build logic (specific for Qt-Extended) from the script runner logic
- fully supports TeamBuilder and can trottle build, moc, and IO tasks differently.
- is fast!!!!

QBuild is a bootstrapping solution, i.e. the only thing you need to do to build Qt-Extended is:
  configure <options>
  make
  make image

Configure will build QBuild, and QBuild will produce very small Makefile's that basically re-direct all make calls back to QBuild. The configuration options are saved in a cache file that is read by QBuild upon any invocation.

Within QBuild, all pro files are named qbuild.pro and there's no need to create template pro files for "SUBDIRS". QBuild will always traverse through the complete source tree and will discover all 'real' projects.
QBuild .pro files are also different from QMake .pro files in that the first can contain scripting language that define the project specific logic.

So far one class has been modified:
- qt4project.cpp/h -> basically the mechanism to discover project files needs to be adapted to QBuild.
There's a bit of nastiness in the code that I'd like to see fixed, esp the ugly global boolean, but I'm not familar enough with the code to do that properly.

How to use?
- You should get yourself a copy of //depot/qtopia/main (from the Brisbane p4.trolltech.com.au:866 server)
- The actual contents of Qtopia are irrelevant at this stage, i.e. you don't need to sync every day. We only use it as a sample of a large project containing qbuild stuff.
- hack the ~/depot/qtopia/main/qbuild.pro file and set "SUBDIRS=examples etc/themes" (we don't want too much stuff at this stage), i.e. make sure you're not scanning 'src' as well.
- build and run Qt Creator and make sure you have your Qt Creator IDE as the FIRST open project (or whatever you use for testing)
- then do a project "Open Project/Session" on ~/depot/qtopia/main/qbuild.pro

What you should get is the usual IDE project tree and then a second one below with the Qtopia stuff.
The Qtopia tree however doesn't show much projects: just the base and then examples.pro and themes.pro (and this depends on these old files still lying around in the depot).

To activate Qbuild support:
- Compile Qt Creator with QTEXTENDED_QBUILD_SUPPORT defined (see top of this file)
- rebuild and run Qt Creator

What you should get is the usual IDE project tree and then a second one below with the Qtopia stuff but this time with approx 15 qbuild.pro projects.
Issues:
* Every project file is still named 'qbuild.pro', I hacked ProFile::ProFile() to set m_displayFileName to the base dir name when it's a qbuild.pro file (i.e. src/applications/addressbook/qbuild.pro should become 'addressbook' instead of'qbuild.pro'), but that doesn't seem to work. Also ProFile is a copy from Qt? so I can't really modify it.
* The tree is (obviously) not showing much depth. This is because there are no .pro files in directories such as src/examples. So what we would need is a way to create virtual projects or something similar.
Any suggestions how to fix this?

*** Future steps ***
- running configure, make, make image
- defining configuration options (i.e. supporting multiple shadow builds)
- running QtExtended on qvfb
- debugging QtExtended on qvfb
- running QtExtended on a device
- debugging QtExtended on a device
- and probably more

*/

#endif
