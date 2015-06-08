This plugin adds support for Team Foundation Server and VS Online to the Unity Version Control Integration.

This plugin is not officially supported by Unity.
It has been developed by Microsoft and is released under public domain license.

TFSPlugin specific bugs should be reported to tfsunity@microsoft.com

Prerequisites
	1. A Unity pro license for Unity 5.x or a Unity team license for Unity 4.x
	2. Visual studio 2013 or Team Explorer 2013

Plugin Installation/Upgrade Instructions   
	1. Copy the TfsPlugin.exe and TfsPlugin.exe.config into your "UnityEditor\Editor\Data\Tools\VersionControl" folder

TfsPlugin.exe Configuration
	1. Team wide configuration such as the server URL can be entered into the TfsPlugin.exe.config and checked in
	
Project setup Instructions
	1. Open or create a Unity project located within a TFS workspace folder
	2. Open up Editor settings
		a. Edit->Project Settings->Editor
		b. Note that the editor settings is on the inspector window
	3. On the editor settings panel
		a. Mode - Select "TFS" 
		b. Enter the server URL of your TFS Team Project into the Server text box
		c. Press the "Connect" button
		d. Make sure the plugin status shows as "Connected"

Development Notes
	1. A running plugin will write to a log file named tfsplugin.log in the project's Library folder
	2. Copying new builds of the plugin and .config into a local unity editor as a post build step can save iteration time
	3. Breakpoints can be hit while using the unity editor by uncommenting "Debugger.Launch();" in Program.cs of the TfsPlugin
	4. To run the unit tests in TfsPlugin.UnitTestProject, UnitTestSettings.settings can be edited with the connection information for the target TFS server.
