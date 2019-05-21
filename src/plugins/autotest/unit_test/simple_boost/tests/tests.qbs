import qbs
import qbs.Environment
Project {
    name: "Tests"
    property string boostIncDir: {
        if (typeof Environment.getEnv("BOOST_INCLUDE_DIR") !== 'undefined')
            return Environment.getEnv("BOOST_INCLUDE_DIR");
        return undefined;
    }
    references: [ "deco/deco.qbs", "fix/fix.qbs", "params/params.qbs" ]
}
