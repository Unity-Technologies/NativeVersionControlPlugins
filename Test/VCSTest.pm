use lib 'Test/Perforce';
use PerforceTest;
#use lib 'Subversion';
#use SvnTest;

sub IntegrationTest
{
	PerforceIntegrationTests(@_);
}

1;
