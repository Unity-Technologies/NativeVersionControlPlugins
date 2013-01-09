use warnings;
use strict;
use Getopt::Long;
use Cwd;
use File::Path qw (rmtree mkpath);

my ($target, @configs);
GetOptions("target=s"=>\$target, "configs=s"=>\@configs);
@configs = split(/,/,join(',',@configs));

sub BuildLinux ($);

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

if ($target eq "mac")
{
	BuildMac();	
}
elsif ($target eq "win32")
{
	BuildWin32();	
}
elsif ($target eq "linux32")
{
	BuildLinux ($target);
}
elsif ($target eq "linux64")
{
	BuildLinux ($target);
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

sub BuildWin32
{
  rmtree("Build");
  system("msbuilder.cmd", "VersionControl.sln", "P4Plugin", "Win32") && die ("Failed to build PerforcePlugin.exe");
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
