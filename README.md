#### About

This is the source code of the builtin version control plugins that are shipped with [Unity](http://www.unity3d.com).

You can build support for you own favourite version control system into Unity by cloning this repository and make changes as needed.

Note that only Perforce support is shipped for Unity 4.2+, PlasticSCM for 4.3+, TFSPlugin for 4.6+.

#### Overview

A plugin is an executable located in a designated directory that Unity can start and kill at will. At startup Unity will scan the directory and start each executable in order to identify the plugin and its settings. When a version control system has been enabled in unity it will start the associated plugin executable and send it commands using stdin/stdout (MacOS) or Named Pipes (Windows).

The Perforce plugin is using the perforce provided libraries and its callback style API. Furthermore is streams results from the perforce server directly to Unity.

You need Unity 4.2+ to use the integrated version control plugins.

#### Structure

* Common/ contains structures and functionallity common to all plugins
* P4Plugin/ contains the Perforce plugin code and Perforce libraries (binaries shipped with Unity)
* PlasticSCMPlugn contains the Plastic plugin code (binaries shipped with Unity)
* TFSPlugin/ contains the Team Foundation Server/VS Online plugin code
* Test/ contains integration tests

To build:
perl ./build.pl  

To test:
perl ./build.pl -test

#### License and terms

The plugin code itself is licensed under public domain. 

Libraries used by the plugin have their own licenses and are allowed to be distributed with the plugin.

The PerforcePlugin uses the P4 API, for more information on terms of usage and how to get the P4 API visit the following links
http://www.perforce.com/downloads/terms-use
http://www.perforce.com/product/components/apis
