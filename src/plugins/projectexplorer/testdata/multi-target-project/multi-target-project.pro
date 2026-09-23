TEMPLATE = subdirs
app.file = multi-target-project-app.pro
lib.file = multi-target-project-lib.pro
dyn.file = multi-target-project-dyn.pro
dyn.depends = lib
SUBDIRS = app lib dyn
