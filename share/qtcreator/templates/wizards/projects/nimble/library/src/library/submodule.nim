# This is just an example to get you started. Users of your library will
# import this file by writing ``import %{ProjectName}/submodule``. Feel free to rename or
# remove this file altogether. You may create additional modules alongside
# this file as required.

type
  Submodule* = object
    name*: string

proc initSubmodule*(): Submodule =
  ## Initialises a new ``Submodule`` object.
  Submodule(name: "Anonymous")
