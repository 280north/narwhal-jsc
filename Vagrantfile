# -*- mode: ruby -*-
# vi: set ft=ruby :

def narwhal_home
  [
    ENV["NARWHAL_HOME"],
    File.dirname(File.dirname(`which narwhal`)),
    File.dirname(File.dirname(Dir.pwd))
  ].each do |dir|
    return dir unless dir.nil? or !File.executable?(File.join(dir, "bin", "narwhal"))
  end
  nil
end

def cappuccino_home
  File.join(File.dirname(narwhal_home), "cappuccino")
end

Vagrant::Config.run do |config|
    # All Vagrant configuration is done here. The most common configuration
  # options are documented and commented below. For a complete reference,
  # please see the online documentation at vagrantup.com.

  # Every Vagrant virtual environment requires a box to build off of.
  config.vm.box = "lucid32"

  # The url from where the 'config.vm.box' box will be fetched if it
  # doesn't already exist on the user's system.
  config.vm.box_url = "http://files.vagrantup.com/lucid32.box"

  # Boot with a GUI so you can see the screen. (Default is headless)
  # config.vm.boot_mode = :gui

  # Assign this VM to a host-only network IP, allowing you to access it
  # via the IP. Host-only networks can talk to the host machine as well as
  # any other machines on the same network, but cannot be accessed (through this
  # network interface) by any external networks.
  # config.vm.network :hostonly, "33.33.33.10"

  # Assign this VM to a bridged network, allowing you to connect directly to a
  # network using the host's network device. This makes the VM appear as another
  # physical device on your network.
  # config.vm.network :bridged

  # Forward a port from the guest to the host, which allows for outside
  # computers to access the VM, whereas host only networking does not.
  # config.vm.forward_port 80, 8080

  # Share an additional folder to the guest VM. The first argument is
  # an identifier, the second is the path on the guest to mount the
  # folder, and the third is the path on the host to the actual folder.
  
  puts cappuccino_home
  
  config.vm.share_folder "narwhal-jsc", "/narwhal-jsc", Dir.pwd
  config.vm.share_folder "narwhal", "/narwhal", narwhal_home unless narwhal_home.nil?
  config.vm.share_folder "cappuccino", "/cappuccino", cappuccino_home unless cappuccino_home.nil?

  # Provision

  config.vm.provision :shell, :inline => <<-eos
    #!/bin/bash
    
    set -u
    set -e

    apt-get update
    apt-get -y install build-essential libwebkit-dev xvfb

    echo 'export NARWHAL_HOME="/narwhal"'                         >> /home/vagrant/.profile
    echo 'export NARWHAL_ENGINE="jsc"'                            >> /home/vagrant/.profile
    echo 'export PATH="$PATH:$NARWHAL_HOME/bin:/narwhal-jsc/bin"' >> /home/vagrant/.profile
    echo 'export DISPLAY=:1'                                      >> /home/vagrant/.profile
    echo 'killall Xvfb'                                           >> /home/vagrant/.profile
    echo 'nohup Xvfb :1 -screen 0 1024x768x16 &'                  >> /home/vagrant/.profile

  eos
end
