Narwhal JavaScriptCore / JSCocoa Platform
=========================================

Setup
-----

    git clone git://github.com/tlrobinson/narwhal.git narwhal
    cd narwhal/engines
    git clone git://github.com/tlrobinson/narwhal-jsc.git jsc
    cd jsc
    make
    cd ../..
    NARWHAL_ENGINE=jsc bin/narwhal

Notes
-----

* Only builds on Mac OS X and JSCocoa support is currently broken, but both of these things can be fixed with improved Makefiles (or Jakefiles)