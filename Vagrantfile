# -*- mode: ruby -*-
# vi: set ft=ruby :

# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure(2) do |config|
  # The most common configuration options are documented and commented below.
  # For a complete reference, please see the online documentation at
  # https://docs.vagrantup.com.

  # Provider-specific configuration so you can fine-tune various
  # backing providers for Vagrant. These expose provider-specific options.
  config.vm.provider "virtualbox" do |v|
    # Customize the amount of memory on the VM:
    v.memory = 2048
  end

  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://atlas.hashicorp.com/search.
  config.vm.box = "ubuntu/xenial64"

  # Share an additional folder to the guest VM. The first argument is
  # the path on the host to the actual folder. The second argument is
  # the path on the guest to mount the folder. And the optional third
  # argument is a set of non-required options.
  config.vm.synced_folder "./", "/home/vagrant/phaser"

  # Disable automatic box update checking. If you disable this, then
  # boxes will only be checked for updates when the user runs
  # `vagrant box outdated`. This is not recommended.
  # config.vm.box_check_update = false

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # accessing "localhost:8080" will access port 80 on the guest machine.
  # config.vm.network "forwarded_port", guest: 80, host: 8080

  # Create a private network, which allows host-only access to the machine
  # using a specific IP.
  # config.vm.network "private_network", ip: "192.168.33.10"

  # Create a public network, which generally matched to bridged network.
  # Bridged networks make the machine appear as another physical device on
  # your network.
  # config.vm.network "public_network"

  # Define a Vagrant Push strategy for pushing to Atlas. Other push strategies
  # such as FTP and Heroku are also available. See the documentation at
  # https://docs.vagrantup.com/v2/push/atlas.html for more information.
  # config.push.define "atlas" do |push|
  #   push.app = "YOUR_ATLAS_USERNAME/YOUR_APPLICATION_NAME"
  # end

  # Enable provisioning with a shell script. Additional provisioners such as
  # Puppet, Chef, Ansible, Salt, and Docker are also available. Please see the
  # documentation for more information about their specific syntax and use.
  config.vm.provision "shell", inline: <<-SHELL
    sudo add-apt-repository ppa:george-edison55/cmake-3.x
    sudo apt-get -y update
    sudo apt-get -y install g++-4.8 g++ git make libboost-atomic-dev libboost-chrono-dev libboost-thread-dev libboost-system-dev libboost-date-time-dev libboost-regex-dev libboost-filesystem-dev libboost-random-dev libboost-serialization-dev libwebsocketpp-dev openssl libssl-dev ninja-build cmake

    # Build the C++ REST SDK ("Casablanca")
    git clone https://github.com/Microsoft/cpprestsdk.git casablanca
    mkdir -p casablanca/Release/build.release
    cd casablanca/Release/build.release
    cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release
    ninja

    # Build the Azure Storage Client Library for C++
    cd /home/vagrant
    git clone https://github.com/Azure/azure-storage-cpp.git
    sudo apt-get -y install libxml++2.6-dev libxml++2.6-doc uuid-dev
    cd /home/vagrant/azure-storage-cpp/Microsoft.WindowsAzure.Storage
    mkdir build.release
    cd build.release
    CASABLANCA_DIR=/home/vagrant/casablanca CXX=g++-4.8 cmake .. -DCMAKE_BUILD_TYPE=Release
    make

    # Set time zone for Vancouver, Canada
    timedatectl set-timezone Canada/Pacific
    SHELL
  end
