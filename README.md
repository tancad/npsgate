NpsGate
=======

Introduction
------------
NpsGate is an implementation of the SmartNet architecture designed in the master's
thesis by Lance Alt titled "Application Transparent HTTP over a Disruption Tolerant
SmartNet". The SmartNet architecture is a modular framework for experimenting with
and deploying network adaptation solutions.

NpsGate implements the SmartNet core along with several plugins that can be arranged
to form packet processing piplines. A plugin API is provided to easily allow third-
party plugins to be incorporated in to the system.


Installation
------------
To compile the NpsGate core from the git repository you need the following tools and
libraries:

  - autoconf / automake / libtool
  - A C++ compiler (only GCC has been tested)
  - Boost C++ libraries (specifically system and threads libraries)
  - libconfig (http://www.hyperrealm.com/libconfig)
  - libcrafter (http://code.google.com/p/libcrafter)
  - libnetfilterqueue-dev (http://www.netfilter.org/projects/libnetfilter_queue/)

Specific plugins may have additional requirements. Once the above library prerequisites
have been met, NpsGate can be compiled as follows:

$ ./autogen.sh
$ ./configure
$ make
$ make install

By default NpsGate is installed in the 'run' directory. The installation directory can
be changed using the 'configure' script.


Configuration
-------------
NpsGate requires a configuration file to run. See examples in the 'docs' directory.


Running
-------
NpsGate can be started with a specific configuration using the following command:

$ sudo ./npsgate -c <path to configuration file>

NpsGate typically must be run as root in order to intercept packets from the kernel.

