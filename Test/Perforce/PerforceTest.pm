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
	$suite = $_[0];
	$dir = $_[1];
	$p4port = $_[2];
	$option = $_[3];
	$filter = $_[4];
	$mfa = index($dir, "MultiFactorAuthentication") > -1;
	$devnull = "> /dev/null 2>&1";

	unless ($option) { $option = "verbose" };
	unless ($filter) { $filter = "" };

	if ($filter ne ""
		&& index($filter, $dir) == -1)
	{
		print "\n[$suite] '",$dir,"' Perforce Integration Tests Ignored (p4port:'",$p4port,"') (Not included in filter).\n";
		return 0;
	}

	print "\n\n[$suite] '",$dir,"' Perforce Integration Tests Running (p4port:'",$p4port,"')\n";

	rmtree("Test/tmp");
	mkdir "Test/tmp";
	mkdir "Test/tmp/testclient";
	mkdir "Test/tmp/testserver";
	$ENV{'VCS_P4ROOT'} = "Test/tmp/testserver";
	$ENV{'VCS_P4PORT'} = "$p4port";
	$ENV{'VCS_P4CLIENTROOT'} = "Test/tmp/testclient";
	$ENV{'VCS_P4CLIENTROOTABS'} = getcwd() . "/" . $ENV{'VCS_P4CLIENTROOT'};
	$ENV{'VCS_P4CLIENT'} = "testclient";
	$ENV{'VCS_P4USER'} = "vcs_test_user";
	$ENV{'P4CHARSET'} = 'utf8';
	$ENV{'VCS_P4PASSWD'} = 'Secret';
	$ENV{'P4EXECABS'} = getcwd() . "/" . $ENV{'P4EXEC'};

	if ($ENV{'TARGET'} eq "win32")
	{
		$ENV{'VCS_P4ROOT'} =~ s/\//\\/g;
		$ENV{'VCS_P4CLIENTROOT'} =~ s/\//\\/g;
		$ENV{'VCS_P4CLIENTROOTABS'} =~ s/\//\\/g;
		$ENV{'P4EXECABS'} =~ s/\//\\/g;
		$devnull = "2>&1>\$null";
	}

	$ENV{'VCS_P4ROOT'} = abs_path($ENV{'VCS_P4ROOT'});

	$pid = SetupServer();
	SetupClient();
	SetupUsers();

	# print "Press ENTER to continue...";
	# <STDIN>;
	if ($mfa)
	{
		print "Setting up Multi Factor Authentication configuration...\n";
		SetupMFATriggers();
		SetupMFAUser();
		RestartServer($pid);
		ConfigureSecurity();
	}
	# print "Press ENTER to continue...";
	# <STDIN>;

	$exitCode = RunTests($suite, $dir, $option, $filter);

	# print "Press ENTER to continue...";
	# <STDIN>;

	TeardownClient();
	TeardownServer($pid);
	return $exitCode;
}

sub RunTests()
{
	$suite = $_[0];
	$dir = $_[1];
	$option = $_[2];
	$filter = $_[3];

	@files = <Test/$dir/*.txt>;

	$total = @files;
	$count = 0;
	$success = 0;
	$failed = 0;
	$ignored = 0;

	$pluginexec = abs_path($ENV{'P4PLUGIN'});
	$testserver = abs_path($ENV{'TESTSERVER'});
	$clientroot = $ENV{'VCS_P4CLIENTROOT'};

	if (not(-e -f -x $testserver))
	{
		print "\n[$suite][$count/$total] Error testserver '$testserver' doesn't exist\n\n";
		return $total;
	}

	$cwd = getcwd();
	foreach $i (@files) {
		$count++;
		if ($filter ne ""
			&& index($i, $filter) == -1)
		{
			$ignored++;
			print "\n[$suite][$count/$total] '",$i,"' Ignored (p4port:'",$p4port,"')\n";
			next;
		}
		print "\n[$suite][$count/$total] '",$i,"' Running (p4port:'",$p4port,"')\n";
		chdir $cwd;
		rmtree( $clientroot, {keep_root => 1} );
		print "Changing working directory to: '", $clientroot,"'\n";
		chdir $clientroot;
		mkdir "./Assets";
		mkdir "./Library";
		AddExclusiveFile();
		$output = `$testserver $pluginexec $cwd $i $option`;
		$res = $? >> 8;
		print $output;
		if ($res == 0)
		{
			$success++;
		}
		elsif ($? == -1)
		{
			print "\n[$suite][$count/$total] '",$i,"' Error running test: $!\n\n";
			chdir $cwd;
			return 1;
		}
		else
		{
			print "\n[$suite][$count/$total] '",$i,"' Failed (p4port:'",$p4port,"')\n\n";
			$failed++;
		}
		RemoveExclusiveFile();
	}
	chdir $cwd;
	if ($ignored > 0)
	{
		print "[$suite] $ignored of $total tests ignored.\n";
	}
	if ($failed > 0)
	{
		print "[$suite] Failed: $failed of $total tests failed.\n";
		print "[$suite]         $success of $total tests passed.\n";
		return $failed;
	}
	print "[$suite] Success: $success of $total tests passed.\n";
	return 0;
}

sub RunCommand
{
	my $command = $_[0];
	print "Running command $command\n";
	# LoginUser($ENV{'VCS_P4USER'}, $ENV{'VCS_P4PASSWD'});
	system("$ENV{'P4EXECABS'} -p $ENV{'VCS_P4PORT'} -u $ENV{'VCS_P4USER'} -P $ENV{'VCS_P4PASSWD'} -c $ENV{'VCS_P4CLIENT'} -d $ENV{'VCS_P4CLIENTROOTABS'} $command");
}

sub AddExclusiveFile
{
	open(FH, '>', 'Assets/exclusivefile.txt') or die $!;
	print(FH 'File with exclusive open file type modifier.');
	close(FH) or die $1;

	RunCommand('add -t text+l Assets/exclusivefile.txt');
	RunCommand('submit -d "Add Assets/exclusivefile.txt." Assets/exclusivefile.txt');
}

sub RemoveExclusiveFile
{
	RunCommand('delete Assets/exclusivefile.txt');
	RunCommand('submit -d "Delete Assets/exclusivefile.txt." Assets/exclusivefile.txt');
}

sub SetupServer
{
	$root = $ENV{'VCS_P4ROOT'};
	my $p4port = $ENV{'VCS_P4PORT'};
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
	print "Starting server '$p4d'\n";
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

sub ConfigureSecurity
{
	# system("$ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u password_vcs_test_user passwd -O Password1 -P aaaa1111");
	# system("$ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u vcs_test_user passwd -O Secret -P aaaa1111");
	# system("$ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u mfa_test_user passwd -O Mfau1111 -P aaaa1111");
	ResetPassword("password_vcs_test_user", "Password1", "aaaa1111");
	ResetPassword("vcs_test_user", "Secret", "aaaa1111");
	ResetPassword("mfa_test_user", "Mfau1111", "aaaa1111");
	$ENV{'VCS_P4PASSWD'} = 'aaaa1111';
	LoginUser("vcs_test_user", "aaaa1111");
	# RunCommand('configure set auth.autologinprompt=0');
	# system("$ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u vcs_test_user -P aaaa1111 configure set auth.autologinprompt=0");
	# RunCommand('configure set security=0');
}

sub ResetPassword()
{
	($user_, $old_, $new_) = @_;
	print "Reseting password for user: $user_ | $old_ | $new_\n";
	open(FD, "| $ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u $user_ passwd $devnull");
	print FD "$old_\n";
	print FD "$new_\n";
	print FD "$new_\n";
	close(FD);
	1;
}

sub LoginUser()
{
	($user_, $pass_) = @_;
	print "Login for user: $user_ | $pass_\n";
	open(FD, "| $ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u $user_ login $devnull");
	print FD "$pass_\n";
	close(FD);
}

sub RestartServer
{
	RunCommand('admin restart');
	sleep(2);
}

sub TeardownServer
{
	($handle) = @_;
	print "Tearing down server $handle\n";
	KillSubProcess($handle);
	waitpid($handle,0);
	sleep(5);
	rmtree("Test/tmp/testclient");
	rmtree("Test/tmp/testserver");
	rmtree("Test/tmp");
}

sub SetupUsers
{
	print "Setting up user password_vcs_test_user by retrieving clients:\n";
	system("$ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u password_vcs_test_user clients");
	print "Users:\n";
	system("$ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u password_vcs_test_user users");
	system("$ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u password_vcs_test_user passwd -O \\? -P Password1");
	$ENV{'VCS_P4USER'} = "vcs_test_user";
	system("$ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u vcs_test_user passwd -O \\? -P Secret");
}

sub SetupMFAUser
{
	print "Setting up user mfa_test_user by retrieving clients:\n";
	system("$ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u mfa_test_user clients");
	system("$ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u mfa_test_user passwd -O \\? -P Mfau1111");

	$USER_SPEC =<<EOF;

User:   mfa_test_user

Email:  mfa@test_user

Update: 2022/07/20 11:40:48

Access: 2022/07/20 11:40:48

FullName:	   Multi Factor Authentication

Password:	   ******

AuthMethod:		perforce+2fa
EOF
	open(FD, "| $ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u mfa_test_user -P Mfau1111 user -f -i ");
	print FD "$USER_SPEC\n";
	close(FD);

	1;
}

sub SetupMFATriggers
{
	my $mfa_script = getcwd() . "/MFA/mfa-trigger.sh";
	if ($ENV{'TARGET'} eq "win32")
	{
		$mfa_script = "PowerShell " . getcwd() . "/MFA/mfa-trigger.ps1";
		$mfa_script =~ s/\//\\/g;
	}

	$TRIGGERS =<<EOF;

Triggers:
	test-pre-2fa auth-pre-2fa auth "$mfa_script -t pre-2fa -e %quote%%email%%quote% -u %user% -h %host%"
	test-init-2fa auth-init-2fa auth "$mfa_script -t init-2fa -e %quote%%email%%quote% -u %user% -h %host% -m %method%"
	test-check-2fa auth-check-2fa auth "$mfa_script -t check-2fa -e %quote%%email%%quote% -u %user% -h %host% -s %scheme% -k %token%"
EOF

	open(FD, "| $ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u mfa_test_user -P Mfau1111 triggers -i ");
	print FD "$TRIGGERS\n";
	close(FD);

	1;
}

sub SetupClient
{
	$root = $ENV{'VCS_P4CLIENTROOTABS'};
	#print "Login in to server\n";
	#system("$ENV{'P4EXEC'} -p $ENV{'P4PORT'} login");
	print "Setting up workspace $ENV{'VCS_P4CLIENT'} in $root\n";
	mkdir $root;
	$SPEC =<<EOF;

Client:$ENV{'VCS_P4CLIENT'}

Update:2013/02/19 09:13:18

Access:2013/06/24 12:38:18

Description:
	Created by $ENV{'VCS_P4USER'}.

Root:$root

Options:noallwrite noclobber nocompress unlocked nomodtime normdir

SubmitOptions:submitunchanged

LineEnd:local

View:
	//depot/... //$ENV{'VCS_P4CLIENT'}/...
	-//depot/Assets/excludedfile.txt  //$ENV{'VCS_P4CLIENT'}/Assets/excludedfile.txt
EOF

	open(FD, "| $ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u vcs_test_user client -i ");
	print FD "$SPEC\n";
	close(FD);

	# print `$ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} clients`;
	1;
}

sub TeardownClient
{
	print "Tearing down workspace $ENV{'VCS_P4CLIENT'}\n";
	system("$ENV{'P4EXEC'} -p $ENV{'VCS_P4PORT'} -u $ENV{'VCS_P4USER'} -P $ENV{'VCS_P4PASSWD'} client -f -d $ENV{'VCS_P4CLIENT'}");
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
