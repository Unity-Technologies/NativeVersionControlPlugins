use lib 'Test/Perforce';
use PerforceTest;
#use lib 'Subversion';
#use SvnTest;

sub IntegrationTest
{
	$exitCode = PerforceIntegrationTests(@_);
	exit $exitCode;
}

1;
