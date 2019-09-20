use lib 'Test/Perforce';
use PerforceTest;

sub IntegrationTest
{
	$exitCode = PerforceIntegrationTests(@_);
	if ($exitCode != 0)
	{
		exit $exitCode;
	}
}

1;
