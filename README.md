#### About

This is the source code of the builtin version control plugins that are shipped with [Unity](http://www.unity3d.com).

You can build support for you own favourite version control system into Unity by cloning this repository and make changes as needed.

#### Overview

A plugin is an executable located in a designated directory that Unity can start and kill at will. At startup Unity will scan the directory and start each executable in order to identify the plugin and its settings. When a version control system has been enabled in unity it will start the associated plugin executable and send it commands using stdin/stdout (MacOS) or Named Pipes (Windows).

The Perforce and Subversion plugins work in very different ways with Subversion being the simplest.

The Subversion plugin is calling the svn executable and parses the output. This result is in turn sent to Unity.

The Perforce plugin is using the perforce provided libraries and its callback style API. Furthermore is streams results from the perforce server directly to Unity.

Note that version control other than Asset Server is not supported per default in current releases of Unity. It is available as a preview feature in 4.x with team license though. You can enable the feature by putting an empty file called 'vcs.txt' in the root of your Unity project folder and restart Unity. This will make the menu "Window -> Version Control" show the new version control window.

#### Structure

* Common/ contains structures and functionallity common to all plugins
* P4Plugin/ contains the Perforce plugin code and Perforce libraries
* SvnPlugin/ contains the Subversion plugin code and Subversion libraries

#### License and terms

The plugin code itself is licensed under public domain. 

Libraries used by the plugin have their own licenses and are allowed to be distributed with the plugin.

The PerforcePlugin uses the P4 API, for more information on terms of usage and how to get the P4 API visit the following links
http://www.perforce.com/downloads/terms-use
http://www.perforce.com/product/components/apis

The SubversionPlugin uses the subversion executable 'svn', for more information on terms of usage and source code visit the following link
http://subversion.apache.org/

