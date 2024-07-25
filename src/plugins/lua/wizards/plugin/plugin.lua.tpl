return {
    Name = "%{ProjectName}",
    Version = "1.0.0",
    CompatVersion = "1.0.0",
    Vendor = "%{VendorName}",
    Copyright = "%{Copyright}",
    License = "%{License}",
    Category = "My Plugins",
    Description = "%{Description}",
    Url = "%{Url}",
    DocumentationUrl = "",
    Experimental = true,
    DisabledByDefault = false,
    LongDescription = [[
This plugin provides some functionality.
You can describe it more here.
    ]],
    Dependencies = {
        { Name = "Lua",  Version = "%{JS: Util.qtCreatorIdeVersion()}" },
    },
    setup = function()
        require 'init'.setup()
    end
} --[[@as QtcPlugin]]
