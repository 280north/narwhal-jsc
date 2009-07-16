Narwhal JavaScriptCore / JSCocoa Platform
=========================================

Setup
-----

    git clone git://github.com/tlrobinson/narwhal.git narwhal
    cd narwhal/platforms
    git clone git://github.com/tlrobinson/narwhal-jsc.git jsc
    cd jsc
    make jscocoa
    cd ../..
    NARWHAL_PLATFORM=jsc bin/narwhal

Example
-------

    o = NSObject.alloc.init
    o.description
    
Better examples forthcoming.
