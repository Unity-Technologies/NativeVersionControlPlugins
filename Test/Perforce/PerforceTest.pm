use File::Path;
use Cwd;
use Cwd 'abs_path';

if ($ENV{'TARGET'} eq "win32")
{
BEGIN {
	eval("use Win32::Process;");
	eval("use Win32;");
}
}

sub PerforceIntegrationTests
{
	$dir = $_[0];
	$p4port = $_[1];
	$option = $_[2];

	unless ($option) { $option = "verbose" };

	print "Running Perforce Integration Tests in dir:'",$dir,"' p4port:'",$p4port,"'\n";

	rmtree("Test/tmp");
	mkdir "Test/tmp";
	mkdir "Test/tmp/testclient";
	mkdir "Test/tmp/testclient/Assets";
	mkdir "Test/tmp/testserver";
	$ENV{'P4ROOT'} = "Test/tmp/testserver";
	$ENV{'P4PORT'} = "$p4port";
	$ENV{'P4CLIENTROOT'} = "Test/tmp/testclient";
	$ENV{'P4CLIENTROOTABS'} = getcwd() . "/" . $ENV{'P4CLIENTROOT'};
	$ENV{'P4CLIENT'} = "testclient";
	$ENV{'P4USER'} = "vcs_test_user";
	$ENV{'P4CHARSET'} = 'utf8';
	$ENV{'P4PASSWD'} = 'secret';
	
	if ($ENV{'TARGET'} eq "win32")
	{
		$ENV{'P4ROOT'} =~ s/\//\\/g;
		$ENV{'P4CLIENTROOT'} =~ s/\//\\/g;
		$ENV{'P4CLIENTROOTABS'} =~ s/\//\\/g;
	}

	$ENV{'P4ROOT'} = abs_path($ENV{'P4ROOT'});

	$pid = SetupServer();
	SetupClient();
	SetupUsers();

	$exitCode = RunTests($dir, $option);

	TeardownClient();
	TeardownServer($pid);
	return $exitCode;
}

sub RunTests()
{
	$dir = $_[0];
	$option = $_[1];

	@files = <Test/$dir/*.txt>;

	$total = 0;
	$success = 0;

	$pluginexec = abs_path($ENV{'P4PLUGIN'});
	$testserver = abs_path($ENV{'TESTSERVER'});
	$clientroot = $ENV{'P4CLIENTROOT'};

	if (not(-e -f -x $testserver))
	{
		print "Error testserver '$testserver' doesn't exist\n";
		return 1;
	}

	$cwd = getcwd();
	print "Changing working directory to: '", $clientroot,"'\n";
	chdir $clientroot;
	foreach $i (@files) {
		rmtree("./Library");
		mkdir "./Library";
		$output = `$testserver $pluginexec $cwd $i $option`;
		$res = $? >> 8;
		print $output;
		if ($res == 0)
		{
			$success++;
		}
		elsif ($? == -1)
		{
			print "Error running test '$i' : $!\n";
			chdir $cwd;
			return 1;
		}
		else
		{
			print "Test failed -> stopping all tests\n";
			chdir $cwd;
			return 1;
		}
		$total++;
	}
	print "Done: $success of $total tests passed.\n";
	chdir $cwd;
	return 0;
}

sub SetupServer
{
	$root = $ENV{'P4ROOT'}; 
	my $p4port = $ENV{'P4PORT'};
	print "Setting server in '$root' listening on port '$p4port'\n";
	rmtree($root);
	mkdir $root;
	if ($p4port =~ /ssl[46]?[46]?:/)
	{
		my $ssldir = "$root/sslkeys";
		mkdir $ssldir;
		system("chmod 700 $ssldir");
		$ENV{'P4SSLDIR'} =$ssldir;
		system("$ENV{'P4EXEC'} set P4SSLDIR $ssldir");
		system("$ENV{'P4DEXEC'} -Gc -r \"$root\"");
		system("$ENV{'P4DEXEC'} -Gf -r \"$root\"");
	}

	my $p4d = $ENV{'P4DEXEC'};
	system("$p4d -xi -r \"$root\"");
	my $pidfile = getcwd() . "/server.pid";
	my $pid = SpawnSubProcess($p4d, " -r \"$root\" -p $p4port --pid-file=$pidfile");
	sleep(2);
	if ($p4port =~ /ssl[46]?[46]?:/)
	{
		$ENV{'P4SSLDIR'} =$ssldir;
		system("$ENV{'P4EXEC'} -p $p4port trust -y -f");
	}
	if ($ENV{'TARGET'} ne "win32")
	{
		$pid = do { local(@ARGV, $/) = $pidfile; <> };
	}
	print "Server started $pid\n";
	return $pid;
}

sub TeardownServer
{
	($handle) = @_;
	print "Tearing down server $handle\n";
	KillSubProcess($handle);
	waitpid($handle,0);
	sleep(5);
	rmtree "Test/tmp/testclient";
	rmtree("Test/tmp/testserver");
	rmtree("Test/tmp");
}

sub SetupUsers
{
	print "Setting up user password_vcs_test_user\n";
	print "Clients:\n";
	print `$ENV{'P4EXEC'} -p $ENV{'P4PORT'} -u password_vcs_test_user clients`;
	print "Users:\n";
	print `$ENV{'P4EXEC'} -p $ENV{'P4PORT'} -u password_vcs_test_user users`;
	system("$ENV{'P4EXEC'} -u password_vcs_test_user passwd -O ? -P Password1");
	$ENV{'P4USER'} = "vcs_test_user";
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
    Created by $ENV{'P4USER'}.

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
