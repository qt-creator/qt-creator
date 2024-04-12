// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lua_global.h"

#include "luaapiregistry.h"
#include "luaengine.h"

#include <utils/layoutbuilder.h>

#include <QAbstractButton>
#include <QAbstractSlider>
#include <QAbstractSpinBox>
#include <QCalendarWidget>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QFocusFrame>
#include <QFrame>
#include <QGroupBox>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QMainWindow>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMetaProperty>
#include <QProgressBar>
#include <QRubberBand>
#include <QSizeGrip>
#include <QSplashScreen>
#include <QSplitterHandle>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QToolBar>
#include <QWizardPage>

#include <fstream>
#include <iostream>
#include <valarray>

namespace Lua::Internal {

QStringList baseClasses(const QMetaObject *metaObject)
{
    QStringList bases;
    const QMetaObject *base = metaObject;
    while ((base = base->superClass())) {
        bases << QString::fromLocal8Bit(base->className());
    }
    return bases;
}

bool isBaseClassProperty(QString name, const QMetaObject *metaObject)
{
    const QMetaObject *base = metaObject;
    while ((base = base->superClass())) {
        for (int i = 0; i < base->propertyCount(); ++i) {
            QMetaProperty p = base->property(i);
            if (QString::fromLocal8Bit(p.name()) == name)
                return true;
        }
    }
    return false;
}

template<class T>
QString createQObjectRegisterCode()
{
    // Add new types here when you've added "sol_lua_check, sol_lua_get and sol_lua_push" for them.
    // clang-format off
    static const QStringList whiteListedTypes = {
        "bool", "int", "double", "float",
        "QString", "QRect"};
    // clang-format on

    auto &metaObject = T::staticMetaObject;
    auto className = QString::fromLocal8Bit(metaObject.className());

    QStringList parts = {};

    // properties
    for (int i = 0; i < metaObject.propertyCount(); ++i) {
        QMetaProperty p = metaObject.property(i);
        QMetaType t = p.metaType();
        QString typeName = QString::fromLocal8Bit(t.name());
        QString propName = QString::fromLocal8Bit(p.name());
        if (isBaseClassProperty(propName, &metaObject))
            continue;

        if (!p.isEnumType() && !whiteListedTypes.contains(typeName)) {
            qDebug() << "Skipping" << p.name() << "of type" << typeName
                     << "as it is not whitelisted";
            continue;
        }

        QString propTemplate
            = QString("    \"%1\",\nsol::property(").arg(QString::fromLocal8Bit(p.name()));

        if (p.isReadable()) {
            QString readTemplate;

            if (p.isEnumType()) {
                readTemplate = QString(R"([](const %1 *obj) -> const char * {
                    auto p = %1::staticMetaObject.property(%2);
                    int v = p.read(obj).toInt();
                    return p.enumerator().valueToKey(v);
            })")
                                   .arg(className)
                                   .arg(i);

            } else {
                readTemplate = QString(R"([](const %1 *obj) -> %2 {
    return qvariant_cast<%2>(
        %1::staticMetaObject.property(%3).read(obj));
    })")
                                   .arg(className)
                                   .arg(typeName)
                                   .arg(i);
            }

            propTemplate += readTemplate;
        }

        if (p.isWritable()) {
            QString writeTemplate = ",\n";

            if (p.isEnumType()) {
                writeTemplate += QString(R"([](%1 *obj, const char *v) {
                    auto p = %1::staticMetaObject.property(%2);
                    int i = p.enumerator().keyToValue(v);
                    p.write(obj, i);
            })")
                                     .arg(className)
                                     .arg(i);

            } else {
                writeTemplate += QString(R"([](%1 *obj, %2 v) {
    %1::staticMetaObject.property(%3).write(obj, QVariant::fromValue(v));
    })")
                                     .arg(className)
                                     .arg(typeName)
                                     .arg(i);
            }

            propTemplate += writeTemplate;
        }

        propTemplate += ")";

        parts << propTemplate;
    }

    // Methods

    /*for (int i = 0; i < metaObject.methodCount(); i++) {
        QMetaMethod m = metaObject.method(i);
        QString methodTemplate;
        if (m.methodType() == QMetaMethod::Signal) {
            QString name = QString::fromLocal8Bit(m.name());
            name[0] = name[0].toUpper();
            name = "on" + name;

            methodTemplate = QString("    \"%1\",\n").arg(name);



        } else if (m.methodType() == QMetaMethod::Slot) {
        } else if (m.methodType() == QMetaMethod::Method) {
        } else if (m.methodType() == QMetaMethod::Constructor) {
        } else {
            qDebug() << "Unknown method type" << m.methodType();
        }

        templateString += methodTemplate;
    }*/

    // Generate base classes
    if (metaObject.superClass()) {
        parts << QString("sol::base_classes,\nsol::bases<%1>()")
                     .arg(baseClasses(&metaObject).join(','));
    }

    const QString registerFunctionTemplate = QString(R"(
static void register%1Bindings(sol::state &lua) {
    lua.new_usertype<%1>("%1",
                         %2
    );
}
    )")
                                                 .arg(className)
                                                 .arg(parts.join(",\n"));

    return registerFunctionTemplate;
}

template<class... Classes>
QStringList createQObjectRegisterCodes()
{
    QStringList result;
    ((result << createQObjectRegisterCode<Classes>()), ...);
    return result;
}

template<class... Classes>
QStringList createRegisterCalls()
{
    QStringList result;
    ((result << QString("register%1Bindings(lua);")
                    .arg(QString::fromLocal8Bit(Classes::staticMetaObject.className()))),
     ...);
    return result;
}

template<class... Classes>
QStringList createIncludeCalls()
{
    QStringList result;
    ((result << QString("#include <%1>")
                    .arg(QString::fromLocal8Bit(Classes::staticMetaObject.className()))),
     ...);
    return result;
}

template<class... Classes>
QString createBindings()
{
    QStringList registerFunctions = createQObjectRegisterCodes<Classes...>();

    QString finalFunction = QString(R"(
void registerUiBindings() {
    auto &lua = LuaEngine::instance().lua();
    %1
}
    )")
                                .arg(createRegisterCalls<Classes...>().join('\n'));

    return QString(R"(
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lua_global.h"
#include "luaengine.h"

#include <QMetaProperty>

%1

namespace Lua::Internal
{
    %2

    %3
}
    )")
        .arg(createIncludeCalls<Classes...>().join('\n'))
        .arg(registerFunctions.join('\n'))
        .arg(finalFunction);
}

void createWidgetBindings()
{
    std::ofstream out("/tmp/bindings.cpp", std::ios::out | std::ios::trunc);

    out << createBindings<QObject,
                          QWidget,
                          QDialog,
                          QAbstractButton,
                          QAbstractSlider,
                          QAbstractSpinBox,
                          QCalendarWidget,
                          QComboBox,
                          QDialogButtonBox,
                          QDockWidget,
                          QFocusFrame,
                          QFrame,
                          QGroupBox,
                          QKeySequenceEdit,
                          QLineEdit,
                          QMainWindow,
                          QMdiSubWindow,
                          QMenu,
                          QMenuBar,
                          QProgressBar,
                          QRubberBand,
                          QSizeGrip,
                          QSplashScreen,
                          QSplitterHandle,
                          QStatusBar,
                          QTabBar,
                          QTabWidget,
                          QToolBar,
                          QWizardPage>()
               .toStdString()
        << std::endl;

    qDebug() << "Done!";
}

template<typename... T>
std::valarray<QString> createTypeBinding(QString typeName, QString fieldTypeName, T... fields)
{
    QString cppTemplateStr = R"(
// %1
bool sol_lua_check(sol::types<%1>,
                   lua_State *L,
                   int index,
                   std::function<sol::check_handler_type> handler,
                   sol::stack::record &tracking)
{ return sol::stack::check<sol::table>(L, index, handler, tracking); }
%1 sol_lua_get(sol::types<%1>, lua_State *L, int index, sol::stack::record &tracking)
{
    sol::state_view lua(L);
    sol::table table = sol::stack::get<sol::table>(L, index, tracking);
    return %2;
}
int sol_lua_push(sol::types<%1>, lua_State *L, const %1 &value)
{
    sol::state_view lua(L);
    sol::table table = lua.create_table();
    %3;
    return sol::stack::push(L, table);
}
)";

    QString headerTemplateStr = R"(SOL_CONVERSION_FUNCTIONS(%1)
)";

    auto createField = [&](const std::pair<QString, QString> &field) {
        return QString("table.get_or<%1, const char *, %1>(\"%2\", %3)")
            .arg(fieldTypeName)
            .arg(field.first)
            .arg(field.second);
    };
    QString createTypeFromTable = QString("%1(%2)").arg(typeName).arg(
        QStringList{createField(fields)...}.join(','));

    QString createTableFromType
        = QString("table.set(%1)")
              .arg(QStringList{QString(R"("%1", value.%1())").arg(fields.first)...}.join(','));

    return {cppTemplateStr.arg(typeName).arg(createTypeFromTable).arg(createTableFromType),
            headerTemplateStr.arg(typeName)};
}

void createTypeBindings()
{
    std::valarray<QString> code = {"", R"(
#define SOL_CONVERSION_FUNCTIONS(TYPE) \
    bool LUA_EXPORT sol_lua_check(sol::types<TYPE>, \
                                  lua_State *L, \
                                  int index, \
                                  std::function<sol::check_handler_type> handler, \
                                  sol::stack::record &tracking); \
    TYPE LUA_EXPORT sol_lua_get(sol::types<TYPE>, \
                                lua_State *L, \
                                int index, \
                                sol::stack::record &tracking); \
    int LUA_EXPORT sol_lua_push(sol::types<TYPE>, lua_State *L, const TYPE &rect);

    SOL_CONVERSION_FUNCTIONS(QString)

)"};

    using T = std::pair<QString, QString>;

    code += createTypeBinding("QRect",
                              "int",
                              T{"x", "0"},
                              T{"y", "0"},
                              T{"width", "0"},
                              T{"height", "0"});
    code += createTypeBinding("QSize", "int", T{"width", "0"}, T{"height", "0"});
    code += createTypeBinding("QPoint", "int", T{"x", "0"}, T{"y", "0"});

    code += createTypeBinding("QRectF",
                              "qreal",
                              T{"x", "0.0"},
                              T{"y", "0.0"},
                              T{"width", "0.0"},
                              T{"height", "0.0"});
    code += createTypeBinding("QSizeF", "qreal", T{"width", "0.0"}, T{"height", "0.0"});
    code += createTypeBinding("QPointF", "qreal", T{"x", "0.0"}, T{"y", "0.0"});

    code += createTypeBinding("QColor",
                              "int",
                              T{"red", "0"},
                              T{"green", "0"},
                              T{"blue", "0"},
                              T{"alpha", "255"});

    code[1] += "\n#undef SOL_CONVERSION_FUNCTIONS\n";

    qDebug().noquote() << code[0];
    qDebug().noquote() << code[1];
}

} // namespace Lua::Internal
