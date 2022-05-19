# NativeVersionControlPlugins

## About

This is the source code of the built-in version control plugins that are shipped with [Unity](http://www.unity3d.com).

You can build support for you own favourite version control system into Unity by cloning this repository and make
changes as needed.

Note that only Perforce support is shipped for Unity 4.2+, TFSPlugin for 4.6+.

## Overview

A plugin is an executable located in a designated directory that Unity can start and kill at will. At startup, Unity
will scan the directory and start each executable in order to identify the plugin and its settings. When a version
control system has been enabled in unity it will start the associated plugin executable and send commands to it
by using stdin/stdout (MacOS) or Named Pipes (Windows).

The Perforce plugin is using the libraries provided by Perforce and its callback style API. Furthermore, is streams
results from the perforce server directly to Unity.

You need Unity 4.2+ to use the integrated version control plugins.

## Structure

* `Common/` contains structures and functionallity common to all plugins
* `P4Plugin/` contains the Perforce plugin code and Perforce libraries (binaries shipped with Unity)
* `TFSPlugin/` contains the Team Foundation Server/VS Online plugin code
* `Test/` contains integration tests

To build:

```bash
perl ./build.pl  
```

To test:

```bash
perl ./build.pl -test
```

You need to clone `PerforceBinaries` from mercurial to get the binaries you will run the tests on. You can execute this command from NativeVersionControlPlugins root:
```bash
hg clone --config extensions.largefiles= http://hg-mirror-slo.hq.unity3d.com/unity-extra/perforce PerforceBinaries
```

Steps for setting up Mercurial can be found here: [Setting Up Mercurial](https://confluence.unity3d.com/display/DEV/Setting+Up+Mercurial)

###  Perforce

The Perforce plugin source code is located under `/P4Plugin/Source`. It references the Perforce APIs, located under
`/P4Plugin/Source/r19.1`. As its name states, we're targeting the 19.1 release of Perforce.

Perforce API includes and libraries were downloaded from the
[Perforce downloads page](http://filehost.perforce.com/perforce/r19.1/).

####  Windows

Windows binaries are located under `/P4Plugin/Source/r17.2/lib/win32` and `/P4Plugin/Source/r17.2/lib/win32debug`.

Both directories contain libraries for Win32 - x86 only. They require Visual Studio v10.0 (2010).

## License and terms

The plugin code itself is licensed under public domain.

Libraries used by the plugin have their own licenses and are allowed to be distributed with the plugin.

The PerforcePlugin uses the P4 API. For more information on terms of usage and how to get the P4 API,
visit the following links:

* [Terms of use](http://www.perforce.com/downloads/terms-use)
* [APIs](http://www.perforce.com/product/components/apis)
