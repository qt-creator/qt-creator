/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QObject>

#define Macro

class Class {
    Q_OBJECT
public:
    Q_PROPERTY(bool property READ readProperty CONSTANT);
    void publicFunction();
    void static publicStaticFunction();
    template<int> void publicTemplateFunction();
    template<int> void static publicStaticTemplateFunction();

    int publicVariable;
    int static publicStaticVariable;

signals:
    void signal();

public slots:
    void publicSlot();
    template<int> void publicTemplateSlot();

protected:
    void protectedFunction();
    void static protectedStaticFunction();
    template<int> void protectedTemplateFunction();
    template<int> void static protectedStaticTemplateFunction();

    int protectedVariable;
    int static protectedStaticVariable;

protected slots:
    void protectedSlot();
    template<int> void protectedTemplateSlot();

private:
    void privateFunction();
    void static privateStaticFunction();
    // https://bugreports.qt.io/browse/QTCREATORBUG-20761
    template<int> void privateTemplateFunction();
    template<int> void static privateStaticTemplateFunction();

private slots:
    void privateSlot();
    template<int> void privateTemplateSlot();

    int privateVariable;
    int static privateStaticVariable;
};

template <int>
class TemplateClass{
};

struct Struct {};

template <int>
struct TemplateStruct {};

enum Enum {
    EnumKey
};

namespace NameSpace {}
