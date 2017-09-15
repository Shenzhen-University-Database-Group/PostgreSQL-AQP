use strict;
use warnings;
use TestLib;
use PostgresNode;
use Test::More tests => 14;

my ($slapd, $ldap_bin_dir, $ldap_schema_dir);

$ldap_bin_dir = undef;			# usually in PATH

if ($^O eq 'darwin')
{
	$slapd = '/usr/local/opt/openldap/libexec/slapd';
	$ldap_schema_dir = '/usr/local/etc/openldap/schema';
}
elsif ($^O eq 'linux')
{
	$slapd = '/usr/sbin/slapd';
	$ldap_schema_dir = '/etc/ldap/schema' if -f '/etc/ldap/schema';
	$ldap_schema_dir = '/etc/openldap/schema' if -f '/etc/openldap/schema';
}
elsif ($^O eq 'freebsd')
{
	$slapd = '/usr/local/libexec/slapd';
	$ldap_schema_dir = '/usr/local/etc/openldap/schema';
}

# make your own edits here
#$slapd = '';
#$ldap_bin_dir = '';
#$ldap_schema_dir = '';

$ENV{PATH} = "$ldap_bin_dir:$ENV{PATH}" if $ldap_bin_dir;

my $ldap_datadir = "${TestLib::tmp_check}/openldap-data";
my $slapd_conf = "${TestLib::tmp_check}/slapd.conf";
my $slapd_pidfile = "${TestLib::tmp_check}/slapd.pid";
my $slapd_logfile = "${TestLib::tmp_check}/slapd.log";
my $ldap_conf = "${TestLib::tmp_check}/ldap.conf";
my $ldap_server = 'localhost';
my $ldap_port = int(rand() * 16384) + 49152;
my $ldap_url = "ldap://$ldap_server:$ldap_port";
my $ldap_basedn = 'dc=example,dc=net';
my $ldap_rootdn = 'cn=Manager,dc=example,dc=net';
my $ldap_rootpw = 'secret';
my $ldap_pwfile = "${TestLib::tmp_check}/ldappassword";

note "setting up slapd";

append_to_file($slapd_conf,
qq{include $ldap_schema_dir/core.schema
include $ldap_schema_dir/cosine.schema
include $ldap_schema_dir/nis.schema
include $ldap_schema_dir/inetorgperson.schema

pidfile $slapd_pidfile
logfile $slapd_logfile

access to *
        by * read
        by anonymous auth

database ldif
directory $ldap_datadir

suffix "dc=example,dc=net"
rootdn "$ldap_rootdn"
rootpw $ldap_rootpw});

mkdir $ldap_datadir or die;

system_or_bail $slapd, '-f', $slapd_conf, '-h', $ldap_url;

END
{
	kill 'INT', `cat $slapd_pidfile` if -f $slapd_pidfile;
}

append_to_file($ldap_pwfile, $ldap_rootpw);
chmod 0600, $ldap_pwfile or die;

$ENV{'LDAPURI'} = $ldap_url;
$ENV{'LDAPBINDDN'} = $ldap_rootdn;

note "loading LDAP data";

system_or_bail 'ldapadd', '-x', '-y', $ldap_pwfile, '-f', 'authdata.ldif';
system_or_bail 'ldappasswd', '-x', '-y', $ldap_pwfile, '-s', 'secret1', 'uid=test1,dc=example,dc=net';
system_or_bail 'ldappasswd', '-x', '-y', $ldap_pwfile, '-s', 'secret2', 'uid=test2,dc=example,dc=net';

note "setting up PostgreSQL instance";

my $node = get_new_node('node');
$node->init;
$node->start;

$node->safe_psql('postgres', 'CREATE USER test0;');
$node->safe_psql('postgres', 'CREATE USER test1;');
$node->safe_psql('postgres', 'CREATE USER "test2@example.net";');

note "running tests";

sub test_access
{
	my ($node, $role, $expected_res, $test_name) = @_;

	my $res = $node->psql('postgres', 'SELECT 1', extra_params => [ '-U', $role ]);
    is($res, $expected_res, $test_name);
}

note "simple bind";

unlink($node->data_dir . '/pg_hba.conf');
$node->append_conf('pg_hba.conf', qq{local all all ldap ldapserver=$ldap_server ldapport=$ldap_port ldapprefix="uid=" ldapsuffix=",dc=example,dc=net"});
$node->reload;

$ENV{"PGPASSWORD"} = 'wrong';
test_access($node, 'test0', 2, 'simple bind authentication fails if user not found in LDAP');
test_access($node, 'test1', 2, 'simple bind authentication fails with wrong password');
$ENV{"PGPASSWORD"} = 'secret1';
test_access($node, 'test1', 0, 'simple bind authentication succeeds');

note "search+bind";

unlink($node->data_dir . '/pg_hba.conf');
$node->append_conf('pg_hba.conf', qq{local all all ldap ldapserver=$ldap_server ldapport=$ldap_port ldapbasedn="$ldap_basedn"});
$node->reload;

$ENV{"PGPASSWORD"} = 'wrong';
test_access($node, 'test0', 2, 'search+bind authentication fails if user not found in LDAP');
test_access($node, 'test1', 2, 'search+bind authentication fails with wrong password');
$ENV{"PGPASSWORD"} = 'secret1';
test_access($node, 'test1', 0, 'search+bind authentication succeeds');

note "LDAP URLs";

unlink($node->data_dir . '/pg_hba.conf');
$node->append_conf('pg_hba.conf', qq{local all all ldap ldapurl="$ldap_url/$ldap_basedn?uid?sub"});
$node->reload;

$ENV{"PGPASSWORD"} = 'wrong';
test_access($node, 'test0', 2, 'search+bind with LDAP URL authentication fails if user not found in LDAP');
test_access($node, 'test1', 2, 'search+bind with LDAP URL authentication fails with wrong password');
$ENV{"PGPASSWORD"} = 'secret1';
test_access($node, 'test1', 0, 'search+bind with LDAP URL authentication succeeds');

note "search filters";

unlink($node->data_dir . '/pg_hba.conf');
$node->append_conf('pg_hba.conf', qq{local all all ldap ldapserver=$ldap_server ldapport=$ldap_port ldapbasedn="$ldap_basedn" ldapsearchfilter="(|(uid=\$username)(mail=\$username))"});
$node->reload;

$ENV{"PGPASSWORD"} = 'secret1';
test_access($node, 'test1', 0, 'search filter finds by uid');
$ENV{"PGPASSWORD"} = 'secret2';
test_access($node, 'test2@example.net', 0, 'search filter finds by mail');

note "search filters in LDAP URLs";

unlink($node->data_dir . '/pg_hba.conf');
$node->append_conf('pg_hba.conf', qq{local all all ldap ldapurl="$ldap_url/$ldap_basedn??sub?(|(uid=\$username)(mail=\$username))"});
$node->reload;

$ENV{"PGPASSWORD"} = 'secret1';
test_access($node, 'test1', 0, 'search filter finds by uid');
$ENV{"PGPASSWORD"} = 'secret2';
test_access($node, 'test2@example.net', 0, 'search filter finds by mail');

# This is not documented: You can combine ldapurl and other ldap*
# settings.  ldapurl is always parsed first, then the other settings
# override.  It might be useful in a case like this.
unlink($node->data_dir . '/pg_hba.conf');
$node->append_conf('pg_hba.conf', qq{local all all ldap ldapurl="$ldap_url/$ldap_basedn??sub" ldapsearchfilter="(|(uid=\$username)(mail=\$username))"});
$node->reload;

$ENV{"PGPASSWORD"} = 'secret1';
test_access($node, 'test1', 0, 'combined LDAP URL and search filter');
