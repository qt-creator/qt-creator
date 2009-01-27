The subdirectory cpp contains classes for CPP-Editor/Designer integration.
They are dependent on the CPP editor plugin.

Including cpp.pri in designer.pro enables them and defines
CPP_ENABLED.

Resource handling:
------------------

When the editor is opened for the ui file we remember the internal qrc list
read from the ui file (m_originalUiQrcPaths). In next step (a call to
updateResources()), we detect if the ui file is inside any project or not.

In case the ui file is not in any project (standalone), we apply
m_originalUiQrcPaths to the form.

Otherwise we take the list of qrc files from the project and apply the list
into the form.

Depending if the form is opened in standalone mode or not we save all
resources or only those which are used inside a form (a call to
setSaveResourcesBehaviour()).

We need to update resources of the opened form in following cases:
- a new qrc file is added to / removed from the project in which the form
  exists (we connect filesAdded()/filesRemoved() to the updateResources() slot)
- a new project is added which contains a form (which was opened in standalone
  mode) (we connect foldersAdded()/foldersRemoved() to the updateResources()
  slot)
- the opened form is being saved as... with new name, what causes the editor
  for the form points to a new file which is not in the project (we connect
  changed() to updateResources() slot)

Changes to qrc file contents are not needed to be handled here, since
designer's internal file watcher updates the changes to qrc files silently.

A call to setResourceEditingEnabled(false) removes the edit resources action
form resource browser in designer

Building
--------

The plugin accesses some private headers of Qt Designer 4.5.
Copies of them are located in qt_private to achieve a clean build
from a standard Qt distribution. The script syncqtheader.sh
can be used to update them.
