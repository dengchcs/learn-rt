add_rules("mode.debug", "mode.release")
target("rt")
    set_languages("c++17")
    set_kind("binary")
    add_headerfiles("*.hpp", "deps/*.hpp", "deps/*.h")
    add_files("main.cpp")
    set_warnings("all")
    
    if is_plat("linux") then
        add_syslinks("tbb")
    elseif is_plat("windows") then
        add_toolchains("msvc")
    end

    before_build(function (target) 
        import("core.project.config")
        config.dump()
    end)
    after_build(function (target) 
        import("core.project.config")
        local targetfile = target:targetfile()
        local filepath = path.join(config.buildir(), path.filename(targetfile))
        os.cp(targetfile, filepath)
        print("binary at: %s", filepath)
    end)
