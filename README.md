Narwhal JavaScriptCore / JSCocoa Platform
=========================================

Setup
-----

    git clone git://github.com/tlrobinson/narwhal.git narwhal
    cd narwhal/engines
    git clone git://github.com/tlrobinson/narwhal-jsc.git jsc
    cd jsc
    make

    # run narwhal-jsc directly, defaults to bare JavaScriptCore mode:
    bin/narwhal-jsc
    # equivalent to above:
    bin/narwhal-jscore
    
    # runs with a webkit instance, providing a browser environment (async operations don't work. no run loop)
    bin/narwhal-webkit
    
    # runs with a webkit instance, AppKit setup, runloop running, Web Inspector enabled  
    bin/narwhal-webkit-debug
    # equivalent to above
    NARWHAL_JSC_MODE=webkit-debug bin/narwhal-jsc
    
    cd ../..
    NARWHAL_ENGINE=jsc bin/narwhal
    NARWHAL_ENGINE=jsc NARWHAL_JSC_MODE=webkit-debug bin/narwhal
    # etc ...


Profiler and Debugger
---------------------

In the webkit-debug mode the global object `_inspector` is the [WebInspector object]( http://trac.webkit.org/browser/trunk/WebKit/mac/WebInspector/WebInspector.h). You can call these Objective-C methods by replacing the colons with underscores (the sender argument is ignored and can be omitted).

For example, to enable the debugger (doesn't automatically break though):

    _inspector.startDebuggingJavaScript_();

The following methods are currently exposed through `_inspector`:

    _inspector.show_();
    _inspector.showConsole_();
    _inspector.close_();
    _inspector.attach_();
    _inspector.detach_();

    currentlyDebugging = _inspector.isDebuggingJavaScript();
    _inspector.toggleDebuggingJavaScript_();
    _inspector.startDebuggingJavaScript_();
    _inspector.stopDebuggingJavaScript_();

    enabled = _inspector.isJavaScriptProfilingEnabled();
    _inspector.setJavaScriptProfilingEnabled_(enabled);

    currentlyProfiling = _inspector.isProfilingJavaScript();
    _inspector.toggleProfilingJavaScript_();
    _inspector.startProfilingJavaScript_();
    _inspector.stopProfilingJavaScript_();


Notes
-----

* Only builds on Mac OS X and JSCocoa support is currently broken, but both of these things can be fixed with improved Makefiles (or Jakefiles)