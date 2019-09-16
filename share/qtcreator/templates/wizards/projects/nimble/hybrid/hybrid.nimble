# Package

backend       = "%{ProjectBackend}"
version       = "%{ProjectVersion}"
author        = "%{ProjectAuthor}"
description   = "%{ProjectDescription}"
license       = "%{ProjectLicense}"
srcDir        = "src"
installExt    = @["nim"]
bin           = @["%{ProjectName}"]


# Dependencies

requires "nim >= %{ProjectNimVersion}"
