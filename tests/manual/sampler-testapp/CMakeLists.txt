find_package(Qt6 COMPONENTS Quick3D)

add_qtc_test(sampler-testapp
  MANUALTEST
  CONDITION TARGET Qt::Quick3D
  DEPENDS Qt::Core Qt::Gui Qt::Qml Qt::Quick Qt::Quick3D
  DEFINES QT_QML_DEBUG
  SOURCES main.cpp
)

