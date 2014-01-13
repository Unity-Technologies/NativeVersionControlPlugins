#!/usr/bin/perl
use warnings;
use Getopt::Long;
use File::Path;
use Cwd;

my ($target, $option, $pid, @files);

GetOptions("target=s"=>\$target);

if (not $target)
{
	if ($^O eq "darwin") 
	{
		$target = "macos";		
	}
	elsif ($^O eq "MSWin32") 
	{
		$target = "win32";
	}
}

$ENV{'TARGET'} = $target;
if ($ENV{'TARGET'} eq "win32")
{
	BEGIN 
	{
		eval("use Win32::Process;");
		eval("use Win32;");
	}
}

sub IntegrationTests
{
	$option = $_[0];

	unless ($option) { $option = "verbose" };

	print "Running AES Integration Tests\n";
	mkdir "TestAES";

	$pid = SetupServer();
	sleep(2);
	RunTests($option);
	TeardownServer($pid);
}

sub RunTests()
{
	$option = $_[0];

	@files = <[0-9]*.txt>;

	$total = 0;
	$success = 0;

	$pluginexec = $ENV{'PLUGIN'};
	$testserver = $ENV{'TESTSERVER'};
	$clientroot = $ENV{'CLIENTROOT'};
	
	mkdir $clientroot;

	foreach $i (@files) 
	{
		$output = `$testserver $pluginexec $i $option '$clientroot'`;
		$res = $? >> 8;
		print $output;
		if ($res == 0)
		{
			$success++;
		}
		elsif ($? == -1)
		{
			print "Error running test : $!\n";
			return;
		}
		else
		{
			print "Test failed -> stopping all tests\n";
			return;
		}
		$total++;
	}
	print "Done: $success of $total tests passed.\n";
	
	rmdir $clientroot;
	
	@files = <AssetExchangeServer*>;
	foreach $i (@files) 
	{
		unlink $i;
	}
}

sub SetupServer
{
	return SpawnSubProcess($ENV{'AESSERVER'}, '');
}

sub TeardownServer
{
	($handle) = @_;
	print "Tearing down server $handle\n";
	KillSubProcess($handle);
	waitpid($handle,0);
}

sub ErrorReport
{
	if ($ENV{'TARGET'} eq "win32")
	{
		print Win32::FormatMessage( Win32::GetLastError() );
	}
}

sub SpawnSubProcess
{
	($exec_, $args_) = @_;

	if ($ENV{'TARGET'} eq "win32")
	{
		$ProcessObj = 1;
		Win32::Process::Create($ProcessObj,
							   $exec_,
							   $args_,
							   0,
							   NORMAL_PRIORITY_CLASS,
							   '.') || die ErrorReport();
		return $ProcessObj;
	}
	else
	{
		$pid = fork();
		if ($pid)
		{
			# parent
			return $pid;
		}

		# child
		close STDOUT;
		close STDERR;
		exec("$exec_ $args_") or die "Cannot exec $exec_";
	}
}

sub KillSubProcess
{
	($handle) = @_;
	if ($ENV{'TARGET'} eq "win32")
	{
		$handle->Kill(0);
	}
	else
	{
		kill "KILL", $handle;
	}
}

# Make it general
$ENV{'AESSERVER'} = "/Volumes/Work/Unity/unityAssetServices/start.sh";
$ENV{'PLUGIN'} = "/Volumes/Work/Unity/unityVCPlugins/Build/Debug/AssetExchangeServerPlugin";
$ENV{'TESTSERVER'} = "/Volumes/Work/Unity/unityVCPlugins/Build/Debug/TestServer";
$ENV{'CLIENTROOT'} = "TestAES";

IntegrationTests();