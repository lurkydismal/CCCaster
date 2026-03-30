return {
    -- Called when the mod is loaded
    init = function ()
        Engine.log(Engine.LOG_ERROR, "ON INIT")
    end,

    post_init = function ()
        Engine.log(Engine.LOG_ERROR,"POST INIT CALLED")
    end,

    unload = function ()
        Engine.log(Engine.LOG_ERROR,"UNLOAD CALLED")
    end,

    load = function ()
        Engine.log(Engine.LOG_ERROR,"LOAD CALLED")
    end,

    quit = function ()
        Engine.log(Engine.LOG_ERROR,"QUIT CALLED")
    end,
}
