/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

class Class {
  int a, b;

  enum zoo { a = 1, b = a + 2 + x::y<10>::value };

  enum {};

  typedef enum { k };

  void foo() {}
  inline void bar() {}

  void another_foo() {
    int a = static_cast<int>(1+2/3*4-5%6+(7&8));
  }

  void test_if() {
    if (a == 10) return 1;
    else if (b == 20) return 2;
    else if (c == 30) { x = 1; }
  }

  void test_while() {
    while (int a = 1) {
      exit();
    }

    while (x==1) do_something_here();

    while (x==2) if(a==1) c(); else if (a==2) c(); else c3();
  }

  void test_switch() {
    switch (int k = 1) {
    case 'a': case 'b': case '\\':
      return 1;
    case 1|2: { return 3; } break;
    case x: break;
    case y:
      foo();
      break;
    default:
      return 2;
    }
    s = L"ci\"aa\"ao" L"blah!";
    s2 = "ciao \"ciao\" ciao";
  }
};

class Derived: public Class {
  operator bool() const volatile throw () { return 1; }
  Derived &operator++() {}
};

class Derived2: public Class, public virtual Derived {};
