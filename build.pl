#!/usr/bin/perl
use warnings;
use strict;
use Getopt::Long;
use Cwd;
use File::Path qw (rmtree mkpath);
use lib 'Test';
use VCSTest;
use File::Find;
use File::Basename;

my ($testoption,$test, $target, $clean, @configs, $filter);
GetOptions("test"=>\$test, "testoption=s"=>\$testoption, "filter=s"=>\$filter,
		   "target=s"=>\$target, "configs=s"=>\@configs, "clean"=>\$clean);
@configs = split(/,/,join(',',@configs));

sub BuildLinux ($);
sub TestLinux ($);
sub GetMacArchitecture;

$testoption = "nonverbose" unless ($testoption);

if ($clean)
{
	rmtree("Debug");
	rmtree("Release");
	rmtree("Build");
	find(\&wanted, "./");
	sub wanted
	{
		my($filename, $dirs, $suffix) = fileparse($File::Find::name, qr/\.[^.]*/);
		if (($suffix eq ".o") or ($suffix eq ".obj"))
		{
			print "delete $File::Find::name","\n";
			unlink($_);
		}
	}
	unlink("PerforcePlugin");
	exit 0;
}

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
	elsif ($^O eq "linux")
	{
		$target = "linux64";
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
		# Need to test both architectures if possible
		my $arch = GetMacArchitecture();
		print "Running tests for macOS architecture: $arch\n";
		
		if ($arch eq "arm64")
		{
			print "Testing both architectures on ARM64 machine...\n";
			print "Running x64 tests...\n";
			TestMacX64();
			print "Running ARM64 tests...\n";
			TestMacArm64();
		}
		elsif ($arch eq "x86_64")
		{
			print "Testing only x64 architecture on Intel machine...\n";
			TestMacX64();
		}
		else
		{
			die ("Unknown macOS architecture: $arch");
		}
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

sub TestPerforce()
{
	my $failed = 0;
	$failed += IntegrationTest("1-7", "Plugin", "localhost:1667", $testoption, $filter);
	$failed += IntegrationTest("2-7", "Plugin", "ssl:localhost:1667", $testoption, $filter);
	$failed += IntegrationTest("3-7", "Perforce/Common", "localhost:1667", $testoption, $filter);
	$failed += IntegrationTest("4-7", "Perforce/Common", "ssl:localhost:1667", $testoption, $filter);
	$failed += IntegrationTest("5-7", "Perforce/BaseIPv4", "tcp4:localhost:1667", $testoption, $filter);
	$failed += IntegrationTest("6-7", "Perforce/SecureBaseIPv4", "ssl4:localhost:1667", $testoption, $filter);
	$failed += IntegrationTest("7-7", "Perforce/SquareBracketIPv4", "tcp4:[localhost]:1667", $testoption, $filter);
	# Only works locally, not in CI
	# $failed += IntegrationTest("8-7", "Perforce/MultiFactorAuthentication", "localhost:1667", $testoption, $filter);
	# Only works if DNS routes via IPv6
	# $failed += IntegrationTest("9-7", "Perforce/BaseIPv6", "tcp6:[localhost]:1667", $testoption, $filter);
	# Does not work in new version of Perforce server
	# $failed += IntegrationTest("10-7", "Perforce/SquareBracketIPv6", "tcp6:[::1]:1667", $testoption, $filter);
	# $failed += IntegrationTest("11-7", "Perforce/SecureSquareBracketIPv6", "ssl6:[::1]:1667", $testoption, $filter);

	if ($failed > 0)
	{
		print "\nFAILURE $failed Perforce Integrations Test(s) failed!\n\n";
		exit 1;
	}
	else
	{
		print "\nSUCCESS: All Perforce Integrations Tests passed\n\n";
	}
}

sub BuildMac
{
	rmtree("Build");
	system("make", "-f", "Makefile.osx", "clean-objs") && die ("Failed to clean build artifacts");

	my $arch = GetMacArchitecture();
	print "Detected macOS architecture: $arch\n";
	
	if ($arch eq "arm64")
	{
		print "Building for both x64 and arm64 architectures on ARM64 machine...\n";
		
		# Build x64 first
		print "Building x64 version...\n";
		$ENV{'BUILD_TARGET'} = "P4PluginX64";
		system("make", "-f", "Makefile.osx", "all") && die ("Failed to build macOS x64 version control plugin");

		# In between architectures clean up object files so they are not reused.
		print "Cleaning object files between architecture builds...\n";
		system("make", "-f", "Makefile.osx", "clean-objs") && die ("Failed to clean build artifacts");

		# Build arm64
		print "Building arm64 version...\n";
		$ENV{'BUILD_TARGET'} = "P4PluginArm64";
		system("make", "-f", "Makefile.osx", "all") && die ("Failed to build macOS arm64 version control plugin");

		# Create universal binaries
		print "Creating universal binaries with lipo...\n";
		system("mkdir -p Build/OSX");
		system("lipo", "-create", "Build/OSXarm64/PerforcePlugin", "Build/OSXx64/PerforcePlugin", "-output", "Build/OSX/PerforcePlugin") && die ("Failed to create a universal macOS build version control plugin");
		system("lipo", "-create", "Build/OSXarm64/TestServer", "Build/OSXx64/TestServer", "-output", "Build/OSX/TestServer") && die ("Failed to create a universal macOS test server");
	}
	elsif ($arch eq "x86_64")
	{
		print "Building only x64 architecture on Intel machine...\n";
		$ENV{'BUILD_TARGET'} = "P4PluginX64";
		system("make", "-f", "Makefile.osx", "all") && die ("Failed to build macOS x64 version control plugin");
		
		print "Copying x64 binaries to Build/OSX/...\n";
		system("mkdir -p Build/OSX");
		system("cp", "Build/OSXx64/PerforcePlugin", "Build/OSX/PerforcePlugin") && die ("Failed to copy x64 PerforcePlugin");
		system("cp", "Build/OSXx64/TestServer", "Build/OSX/TestServer") && die ("Failed to copy x64 TestServer");
	}
	else
	{
		die ("Unknown macOS architecture: $arch");
	}
}

sub TestMacX64
{
	$ENV{'P4DEXEC'} = "PerforceBinaries/OSX/x86_64/p4d";
	$ENV{'P4EXEC'} = "PerforceBinaries/OSX/x86_64/p4";
	$ENV{'P4PLUGIN'} = "Build/OSX/PerforcePlugin";
	$ENV{'TESTSERVER'} = "Build/OSX/TestServer";

	# Teamcity artifacts looses their file attributes on transfer
	chmod 0755, glob("Build/OSX/*");

	print "Running Perforce tests with x64 binaries...\n";
	TestPerforce();
	print "x64 tests completed.\n";
}

sub TestMacArm64
{
	$ENV{'P4DEXEC'} = "PerforceBinaries/OSX/arm64/p4d";
	$ENV{'P4EXEC'} = "PerforceBinaries/OSX/arm64/p4";
	$ENV{'P4PLUGIN'} = "Build/OSX/PerforcePlugin";
	$ENV{'TESTSERVER'} = "Build/OSX/TestServer";

	# Teamcity artifacts looses their file attributes on transfer
	chmod 0755, glob("Build/OSX/*");

	print "Running Perforce tests with ARM64 binaries...\n";
	TestPerforce();
	print "ARM64 tests completed.\n";
}

sub BuildWin32
{
	rmtree("Build");
	system("msbuilder.cmd", "VersionControl.sln", "P4Plugin", "Win32") && die ("Failed to build PerforcePlugin.exe");
	system("msbuilder.cmd", "VersionControl.sln", "TestServer", "Win32") && die ("Failed to build TestServer.exe");
}

sub TestWin32
{
	$ENV{'P4DEXEC'} = 'PerforceBinaries\Win_x64\p4d.exe';
	$ENV{'P4EXEC'} = 'PerforceBinaries\Win_x64\p4.exe';
	$ENV{'P4PLUGIN'} = 'Build\Win32\PerforcePlugin.exe';
	$ENV{'TESTSERVER'} = 'Build\Win32\TestServer.exe';

	TestPerforce();
}

sub BuildLinux ($)
{
	system ('make', '-f', 'Makefile.gnu', 'clean');
	system ('make', '-f', 'Makefile.gnu') && die ("Failed to build PerforcePlugin for linux64");
}

sub TestLinux ($)
{
	$ENV{'P4DEXEC'} = "PerforceBinaries/linux64/p4d";
	$ENV{'P4EXEC'} = "PerforceBinaries/linux64/p4";
	$ENV{'P4PLUGIN'} = "Build/linux64/PerforcePlugin";
	$ENV{'TESTSERVER'} = "Build/linux64/TestServer";

	# Teamcity artifacts looses their file attributes on transfer
	chmod 0755, glob("Build/linux64/*");

	TestPerforce();
}

sub GetMacArchitecture
{
	my $arch = `uname -m`;
	chomp($arch);
	return $arch;
}
