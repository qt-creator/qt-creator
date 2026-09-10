# The package a run hands its application to. Qt Creator running on a HarmonyOS device
# cannot install what it builds, so a run hands the built library over to a package that
# is installed already, and this is that package's application. Everything else about it
# is an ordinary Qt application: the libraries it carries are what the handed-over
# library resolves its dependencies against, so it has to be built with the Qt the
# device builds against.
if (NOT OHOS)
  return()
endif()

set(_runner_sources "${PROJECT_SOURCE_DIR}/share/qtcreator/harmonyos")

# One source of truth for the port. The plugin compiles the same sources on demand and
# passes the same constant.
file(STRINGS "${PROJECT_SOURCE_DIR}/src/plugins/harmonyos/harmonyosconstants.h"
  _channel_port_line REGEX "HARMONYOS_CHANNEL_PORT")
string(REGEX MATCH "[0-9]+" _channel_port "${_channel_port_line}")
if (NOT _channel_port)
  message(FATAL_ERROR "No HARMONYOS_CHANNEL_PORT in harmonyosconstants.h")
endif()

# A device installs a package only under the bundle name its provisioning profile is for,
# and the runner is signed with the same material as everything else here.
set(QTC_HARMONYOS_RUNNER_BUNDLE_NAME "" CACHE STRING
  "Bundle name the runner package is built under, which the provisioning profile decides")

add_qtc_executable(qtcrunner
  SKIP_INSTALL
  DEFINES QTC_CHANNEL_PORT=${_channel_port}
  # Widgets brings Core and Gui. The set is the one a handed-over Qt widget application
  # needs to find in the package, not the one the runner itself uses.
  DEPENDS
    Qt::Widgets Qt::Network Qt::Xml Qt::Concurrent Qt::PrintSupport Qt::Sql
  INCLUDES "${_runner_sources}"
  SOURCES
    "${_runner_sources}/qtcload.cpp"
    "${_runner_sources}/qtcrunner.cpp"
  QT_APP
)

if (NOT TARGET qtcrunner)
  return()
endif()

if (QTC_HARMONYOS_RUNNER_BUNDLE_NAME)
  set_target_properties(qtcrunner PROPERTIES
    QT_HARMONYOS_APP_BUNDLE_NAME "${QTC_HARMONYOS_RUNNER_BUNDLE_NAME}")
endif()

# A want cannot name an ability in another bundle, so Qt Creator reaches the runner
# through an implicit one carrying this scheme, and the runner has to say it answers it.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/qtcrunner-harmonyos-extras.json"
"{
    \"launch-schemes\": [
        \"qtcrun\"
    ]
}
")
