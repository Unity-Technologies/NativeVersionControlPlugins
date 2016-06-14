use lib 'Test/Perforce';
use PerforceTest;
#use lib 'Subversion';
#use SvnTest;

sub IntegrationTest
{
	$exitCode = PerforceIntegrationTests(@_);
	if ($exitCode != 0)
	{
		exit $exitCode;
	}
}

1;
