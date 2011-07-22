TEMPLATE = subdirs
SUBDIRS = components/styleitem

QML_IMPORT_PATH += $$OUT_PWD

OTHER_FILES = develop.qml \
              gettingstarted.qml \
              newssupport.qml \
              welcomescreen.qml \
              widgets/Button.qml \
              widgets/CheckBox.qml \
              widgets/Feedback.qml \
              widgets/RatingBar.qml \
              widgets/ExampleBrowser.qml \
              widgets/LineEdit.qml \
              widgets/ExampleDelegate.qml \
              widgets/LinksBar.qml \
              widgets/HeaderItemView.qml \
              widgets/RecentSessions.qml \
              widgets/RecentProjects.qml \
              widgets/FeaturedAndNewsListing.qml \
              widgets/NewsListing.qml \
              widgets/TabWidget.qml \
              widgets/TagBrowser.qml \
              examples_fallback.xml \
              qtcreator_tutorials.xml
