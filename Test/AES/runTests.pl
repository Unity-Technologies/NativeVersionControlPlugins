#!/usr/bin/perl
use warnings;
use Getopt::Long;
use File::Path;
use Cwd;

my ($target, $test, $option, $pluginexec, $testserverexec, $clientroot, $exec, $args);

GetOptions("target=s"=>\$target, "test=s"=>\$test);

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

if ($target eq "win32")
{
	BEGIN 
	{
		eval("use Win32::Process;");
		eval("use Win32;");
	}
}

sub IntegrationTests()
{
	$option = $_[0];

	unless ($option) { $option = "verbose" };

	print "Running AES Integration Tests\n";
	mkdir "TestAES";

	print "Starting AES server...\n";
	my $pid = SetupServer();
	sleep(2);
	
	RunTests($option);

	print "Stopping AES server...\n";
	TeardownServer($pid);
	sleep(2);
	
	CleanUp();
}

sub RunTests()
{
	$option = $_[0];

	my @files;
	
	if ($test)
	{
		@files = ( $test );
	}
	else
	{
		@files = <[0-9]*.txt>;
	}
	
	my $total = 0;
	my $success = 0;

	mkdir $clientroot;

	foreach $i (@files) 
	{
		my $output = `$testserverexec $pluginexec $i $option '$clientroot'`;
		my $res = $? >> 8;
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
	
}

sub CleanUp
{
	rmdir $clientroot;
	
	@files = <AssetExchangeServer*>;
	foreach $i (@files) 
	{
		unlink $i;
	}
	
	@files = <storage/logs/*.log>;
	foreach $i (@files) 
	{
		unlink $i;
	}
}

sub SetupServer
{
	return SpawnSubProcess($exec, $args);
}

sub TeardownServer
{
	($handle) = @_;
	print "Tearing down server $handle\n";
	KillSubProcess($handle);
	waitpid($handle, 0);
}

sub ErrorReport
{
	if ($target)
	{
		print Win32::FormatMessage( Win32::GetLastError() );
	}
}

sub SpawnSubProcess
{
	($exec_, $args_) = @_;

	if ($target eq "win32")
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
		#close STDOUT;
		#close STDERR;
		exec("$exec_ $args_") or die "Cannot execute $exec_";
	}
}

sub KillSubProcess
{
	($handle) = @_;
	
	if ($target eq "win32")
	{
		$handle->Kill(0);
	}
	else
	{
		kill 'KILL', $handle;
	}
}

# Make it general
$exec = "node";
$args = "/Volumes/Work/Unity/unityAssetServices/app/aes-serve/main.js --configFile /Volumes/Work/Unity/unityVCPlugins/Test/AES/unityServer.json";
$pluginexec = "/Volumes/Work/Unity/unityVCPlugins/Build/Debug/AssetExchangeServerPlugin";
$testserverexec = "/Volumes/Work/Unity/unityVCPlugins/Build/Debug/TestServer";
$clientroot = "TestAES";

IntegrationTests();