use File::Path;
use Cwd;

if ($ENV{'TARGET'} eq "win32")
{
BEGIN {
	eval("use Win32::Process;");
	eval("use Win32;");
}
}

sub PerforceIntegrationTests
{
	$option = $_[0];

	unless ($option) { $option = "verbose" };

	print "Running Perforce Integration Tests\n";
	mkdir "Test/tmp";
	$ENV{'P4ROOT'} = "Test/tmp/testserver";
	$ENV{'P4PORT'} = "localhost:1667";
	$ENV{'P4CLIENTROOT'} = "Test/tmp/testclient";
	$ENV{'P4CLIENTROOTABS'} = getcwd() . "/" . $ENV{'P4CLIENTROOT'};
	$ENV{'P4CLIENT'} = "testclient";
	$ENV{'P4CHARSET'} = 'utf8';
	$ENV{'P4PASSWD'} = 'secret';
	
	if ($ENV{'TARGET'} eq "win32")
	{
		$ENV{'P4ROOT'} =~ s/\//\\/g;
		$ENV{'P4CLIENTROOT'} =~ s/\//\\/g;
		$ENV{'P4CLIENTROOTABS'} =~ s/\//\\/g;
	}

	$pid = SetupServer();
	sleep(1);
	SetupClient();

	RunTests($option);

	TeardownClient();
	TeardownServer($pid);
}

sub RunTests()
{
	$option = $_[0];

	@basefiles = <Test/*.txt>;
	@perforcefiles = <Test/Perforce/*.txt>;
	@files = (@basefiles, @perforcefiles);

	$total = 0;
	$success = 0;

	$pluginexec = $ENV{'P4PLUGIN'};
	$testserver = $ENV{'TESTSERVER'};
	$clientroot = $ENV{'P4CLIENTROOT'};

	foreach $i (@files) {
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
}

sub SetupServer
{
	$root = $ENV{'P4ROOT'}; 
	print "Setting server in $ENV{'P4ROOT'} listening on port 1667\n";
	rmtree($root);
	mkdir $root;
	system("$ENV{'P4DEXEC'} -xi -r \"$root\"");
	return SpawnSubProcess($ENV{'P4DEXEC'}, " -r \"$root\" -p 1667");
}

sub TeardownServer
{
	($handle) = @_;
	print "Tearing down server $handle\n";
	KillSubProcess($handle);
	waitpid($handle,0);
}

sub SetupClient
{
	$root = $ENV{'P4CLIENTROOTABS'};
	#print "Login in to server\n";
	#system("$ENV{'P4EXEC'} -p $ENV{'P4PORT'} login");
	print "Setting up workspace $ENV{'P4CLIENT'} in $root\n";
	mkdir $root;
	$SPEC =<<EOF;

Client:$ENV{'P4CLIENT'}

Update:2013/02/19 09:13:18

Access:2013/06/24 12:38:18

Description:
    Created by $ENV{'USER'}.

Root:$root

Options:noallwrite noclobber nocompress unlocked nomodtime normdir

SubmitOptions:submitunchanged

LineEnd:local

View:
    //depot/... //$ENV{'P4CLIENT'}/...
EOF

	open(FD, "| $ENV{'P4EXEC'} -p $ENV{'P4PORT'} client -i ");
	print FD "$SPEC\n";
	close(FD);

	# print `$ENV{'P4EXEC'} -p $ENV{'P4PORT'} clients`;
	1;
}

sub TeardownClient
{
	print "Tearing down workspace $ENV{'P4CLIENT'}\n";
	system("$ENV{'P4EXEC'} -p $ENV{'P4PORT'} client -f -d $ENV{'P4CLIENT'}");
	#rmtree $ENV{'P4CLIENTROOT'};
	1;
}


sub ErrorReport{
	if ($ENV{'TARGET'} eq "win32")
	{
		print Win32::FormatMessage( Win32::GetLastError() );
	}
	1;
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
			#print "Parent with child pid $pid\n";
			return $pid; # parent
		}

		#print "Child $pid\n";
		# child
		close STDOUT;
		close STDERR;
		exec("$exec_ $args_") or die "Cannot exec $exec_";
	}
	1;
}

sub KillSubProcess
{
	($handle) = @_;
	if ($ENV{'TARGET'} eq "win32")
	{
		$handle->Kill($exitcode);
	}
	else
	{
		kill "KILL", $handle;
	}
	1;
}

1;
