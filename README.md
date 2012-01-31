Narwhal JavaScriptCore / WebKit Platform
========================================

Setup
-----

    git clone git://github.com/280north/narwhal.git
    cd narwhal/packages
    git clone git://github.com/280north/narwhal-jsc.git
    ./configure
    make

Usage
-----
	
The following commands will execute a specific narwhal-jsc mode:

	narwhal-jscore
	NARWHAL_ENGINE=jscore narwhal
	narwhal-webkit
	NARWHAL_ENGINE=webkit narwhal

The following commands will execute the default narwhal-jsc mode (narwhal-webkit by default).

	narwhal-jsc
	NARWHAL_ENGINE=jsc narwhal

The "narwhal-jsc" executable/engine name is deprecated. Use narwhal-jscore or narwhal-webkit.

Misc
----

A Vagrantfile is included for testing on a Linux VM, e.x.

	vagrant up
	vagrant ssh
	cd /narwhal-jsc
	./configure
	make
	narwhal-webkit

You may need to start Xvfb before running narwhal-webkit:

    nohup Xvfb :1 -screen 0 1024x768x16 &
	export DISPLAY=:1
