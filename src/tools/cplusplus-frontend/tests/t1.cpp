/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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
