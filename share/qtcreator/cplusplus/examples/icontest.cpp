// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QObject>

#define Macro

class Class : public QObject {
    Q_OBJECT

public:
    Q_PROPERTY(int property READ publicStaticFunction CONSTANT)
    int publicFunction() { return 0; }
    int static publicStaticFunction()  { return 0; }
    template<int> void publicTemplateFunction();
    template<int> void static publicStaticTemplateFunction();

    int publicVariable;
    int static publicStaticVariable;

signals:
    void signal();

public slots:
    void publicSlot() {}
    // template<int> void publicTemplateSlot() {}

protected:
    void protectedFunction();
    void static protectedStaticFunction();
    template<int> void protectedTemplateFunction();
    template<int> void static protectedStaticTemplateFunction();

    int protectedVariable;
    int static protectedStaticVariable;

protected slots:
    void protectedSlot() {}
    // template<int> void protectedTemplateSlot() {}

private:
    void privateFunction();
    void static privateStaticFunction();
    // https://bugreports.qt.io/browse/QTCREATORBUG-20761
    template<int> void privateTemplateFunction();
    template<int> void static privateStaticTemplateFunction();

private slots:
    void privateSlot() {}
    // template<int> void privateTemplateSlot() {}

private:
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
