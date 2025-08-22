# PerforcePlugin

## About

This is the official **Perforce** plugin that is shipped with [Unity](http://www.unity3d.com), as a built-in Version Control provider.

On Windows it is shipped as `Editor\Data\Tools\VersionControl\PerforcePlugin.exe`.

## Community supported fork

See also the [Community Supported Perforce (Helix Core) plugin for Unity](https://github.com/perforce/unity-p4-plugin)
It is a fork of these code, but allows the linked version of the P4 API to be changed as needed, and provide more recent builds.

## Overview

You can build support for you own favorite version control system into Unity by cloning this repository and make
changes as needed.

A plugin is an executable located in a designated directory that Unity can start and kill at will. At startup, Unity
will scan the directory and start each executable in order to identify the plugin and its settings. When a version
control system has been enabled in unity it will start the associated plugin executable and send commands to it
by using stdin/stdout (MacOS) or Named Pipes (Windows).

The Perforce plugin is using the libraries provided by Perforce and its callback style API. Furthermore, is streams
results from the perforce server directly to Unity.

You need Unity 4.2+ to use the integrated version control plugins.

## Structure

* `Common/` contains structures and functionality common to all plugins
* `P4Plugin/` contains the Perforce plugin code and Perforce libraries (binaries shipped with Unity)
* `TFSPlugin/` contains the Team Foundation Server/VS Online plugin code (not officially supported by Unity, and not shipped with Unity)
* `Test/` contains integration tests

To build:

```bash
perl ./build.pl
```

To test:

```bash
perl ./build.pl -test
```

### Perforce

The Perforce plugin source code is located under `P4Plugin/Source`. It references the Perforce APIs,
located under `P4Plugin/Source/p4api`.

We are targeting the 24.1 release of Perforce API includes and libraries, that were downloaded from
the [Perforce downloads page](http://filehost.perforce.com/perforce/r24.1/).

The `PerforceBinaries` where downloaded the same locations to run the integrations tests on.

```bash
mkdir -p 'PerforceBinaries\Win_x64'
curl -ssL -o 'PerforceBinaries\Win_x64\p4.exe' 'https://filehost.perforce.com/perforce/r24.1/bin.ntx64/p4.exe'
curl -ssL -o 'PerforceBinaries\Win_x64\p4d.exe' 'https://filehost.perforce.com/perforce/r24.1/bin.ntx64/p4d.exe'
```

#### Windows

Windows static libraries are located under `P4Plugin/Source/p4api/lib/win32` and `win32debug`.

Both directories contain libraries for Win32 - x86 only, not x64.
They require Visual Studio v10.0 (2010).

OpenSSL 1.0.1 static libraries are located under `P4Plugin/Source/openssl/lib/win32`.

## License and terms

The plugin code itself is licensed under public domain.

Libraries used by the plugin have their own licenses and are allowed to be distributed with the plugin.

The PerforcePlugin uses the P4 API. For more information on terms of usage and how to get the P4 API,
visit the following links:

* [Terms of use](http://www.perforce.com/downloads/terms-use)
* [APIs](http://www.perforce.com/product/components/apis)
