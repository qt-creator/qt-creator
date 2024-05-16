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
    Experimental = true,
    DisabledByDefault = false,
    LongDescription = [[
This plugin provides some functionality.
You can describe it more here.
    ]],
    Dependencies = {
        { Name = "Core", Version = "%{JS: Util.qtCreatorIdeVersion()}", Required = true },
        { Name = "Lua",  Version = "%{JS: Util.qtCreatorIdeVersion()}", Required = true },
    },
    setup = function()
        require 'init'.setup()
    end
} --[[@as QtcPlugin]]
