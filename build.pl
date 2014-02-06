#!/usr/bin/perl
use warnings;
use strict;
use Getopt::Long;
use Cwd;
use File::Path qw (rmtree mkpath);
use lib 'Test';
use VCSTest;

my ($testoption,$test, $target, @configs);
GetOptions("test"=>\$test, "testoption=s"=>\$testoption,
		   "target=s"=>\$target, "configs=s"=>\@configs);
@configs = split(/,/,join(',',@configs));

sub BuildLinux ($);
sub TestLinux ($);

$testoption = "nonverbose" unless ($testoption);

if (not $target)
{
	if ($^O eq "darwin") 
	{
		$target = "mac";		
	}
	elsif ($^O eq "MSWin32") 
	{
		$target = "win32";
	}
}

$ENV{'TARGET'} = $target;

if ($target eq "mac")
{
	unless ($test)
	{
		BuildMac();	
	}
	else
	{
		TestMac();
	}
}
elsif ($target eq "win32")
{
	unless ($test)
	{
		BuildWin32();	
	}
	else
	{
		TestWin32();
	}
}
elsif ($target eq "linux32")
{
	unless ($test)
	{
		BuildLinux ($target);
	}
	else
	{
		TestLinux ($target);
	}
}
elsif ($target eq "linux64")
{
	unless ($test)
	{
		BuildLinux ($target);
	}
	else
	{
		TestLinux ($target);
	}
}
else 
{
    die ("Unknown platform");
}

sub BuildMac
{
	system ("rm", "-rf", "Build");
	system("make" , "-f", "Makefile.osx", "all") && die ("Failed to build version control plugins");
}

sub TestMac
{
	$ENV{'P4DEXEC'} = "PerforceBinaries/OSX/p4d";
	$ENV{'P4EXEC'} = "PerforceBinaries/OSX/p4";
	$ENV{'P4PLUGIN'} = "Build/OSXi386/PerforcePlugin";
	$ENV{'TESTSERVER'} = "Build/OSXi386/TestServer";

	# Teamcity artifacts looses their file attributes on transfer
	chmod 0755, glob("Build/OSXi386/*");

	IntegrationTest($testoption);
}

sub BuildWin32
{
  rmtree("Build");
  system("msbuilder.cmd", "VersionControl.sln", "P4Plugin", "Win32") && die ("Failed to build PerforcePlugin.exe");
  system("msbuilder.cmd", "VersionControl.sln", "SvnPlugin", "Win32") && die ("Failed to build SubversionPlugin.exe");
  system("msbuilder.cmd", "VersionControl.sln", "PlasticSCMPlugin", "Any CPU") && die ("Failed to build PlasticSCMPlugin.exe");
  system("msbuilder.cmd", "VersionControl.sln", "TestServer", "Win32") && die ("Failed to build TestServer.exe");
}

sub TestWin32
{
	$ENV{'P4DEXEC'} = 'PerforceBinaries\Win_x64\p4d.exe';
	$ENV{'P4EXEC'} = 'PerforceBinaries\Win_x64\p4.exe';
	$ENV{'P4PLUGIN'} = 'Build\Win32\PerforcePlugin.exe';
	$ENV{'TESTSERVER'} = 'Build\Win32\TestServer.exe';
	IntegrationTest($testoption);
}

sub BuildLinux ($)
{
	my $platform = shift;

	my $cflags = '-O3 -fPIC -fexceptions -fvisibility=hidden -DLINUX';
	my $cxxflags = "$cflags -Wno-ctor-dtor-private";
	my $ldflags = '';

	if ($platform eq 'linux32') {
		$cflags = "$cflags -m32";
		$cxxflags = "$cxxflags -m32";
		$ldflags = '-m32';
	}

	$ENV{'CFLAGS'} = $cflags;
	$ENV{'CXXFLAGS'} = $cxxflags;
	$ENV{'LDFLAGS'} = $ldflags;
	$ENV{'PLATFORM'} = $platform;

	system ('make', '-f', 'Makefile.gnu', 'clean');
	system ('make', '-f', 'Makefile.gnu') && die ("Failed to build $platform");
}

sub TestLinux ($)
{
	my $platform = shift;

	# Teamcity artifacts looses their file attributes on transfer
	chmod 0755, glob("Build/OSXi386/$platform/*");
}
