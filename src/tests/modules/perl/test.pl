use strict;
use warnings;

#
# This the remapping of return values
#
use constant {
	RLM_MODULE_REJECT   => 1, # immediately reject the request
	RLM_MODULE_FAIL     => 2, # module failed, don't reply
	RLM_MODULE_OK       => 3, # the module is OK, continue
	RLM_MODULE_HANDLED  => 4, # the module handled the request, so stop
	RLM_MODULE_INVALID  => 5, # the module considers the request invalid
	RLM_MODULE_DISALLOW => 6, # reject the request (user is locked out)
	RLM_MODULE_NOTFOUND => 7, # user not found
	RLM_MODULE_NOOP     => 8, # module succeeded without doing anything
	RLM_MODULE_UPDATED  => 9, # OK (pairs modified)
	RLM_MODULE_TIMEOUT  => 10, # OK (pairs modified)
	RLM_MODULE_NUMCODES => 11  # How many return codes there are
};

# Same as src/include/log.h
use constant {
	L_AUTH         => 2,  # Authentication message
	L_INFO         => 3,  # Informational message
	L_ERR          => 4,  # Error message
	L_WARN         => 5,  # Warning
	L_PROXY        => 6,  # Proxy messages
	L_ACCT         => 7,  # Accounting messages
	L_DBG          => 16, # Only displayed when debugging is enabled
	L_DBG_WARN     => 17, # Warning only displayed when debugging is enabled
	L_DBG_ERR      => 18, # Error only displayed when debugging is enabled
	L_DBG_WARN_REQ => 19, # Less severe warning only displayed when debugging is enabled
	L_DBG_ERR_REQ  => 20, # Less severe error only displayed when debugging is enabled
};

#  Global variables can persist across different calls to the module.
#
#
#	{
#	 my %static_global_hash = ();
#
#		sub post_auth {
#		...
#		}
#		...
#	}

sub authorize {
	my $p = shift();

	return RLM_MODULE_OK;
}

# Function to handle authenticate
sub authenticate {
	my $p = shift();

	# For debugging purposes only
	log_request_attributes($p);

	if ($p->{'request'}{'User-Name'}[0] =~ /^baduser/i) {
		# Reject user and tell him why
		$p->{'reply'}{'Reply-Message'}[0] = "Denied access by rlm_perl function";
		# For testing return NOTFOUND - returning REJECT immediately rejects the packet so fails the test
		return RLM_MODULE_NOTFOUND;
	} else {
		# Accept user and set some attribute
		if (&freeradius::xlat("%request.client('group')") eq 'UltraAllInclusive') {
			# User called from NAS with unlim plan set, set higher limits
			$p->{'reply'}{'Vendor-Specific'}{'Cisco'}{'h323-credit-amount'}[0] = "1000000";
			$p->{'reply'}{'Filter-Id'}[0] = 'Everything'
		} else {
			# Check we received two values for Cisco.AVPair
			if ($p->{'request'}{'Vendor-Specific'}{'Cisco'}{'AVPair'}[1] ne 'is=crazy') {
				return RLM_MODULE_DISALLOW;
			}
			if ($p->{'request'}{'Class'}[0] ne 'abcdef') {
				return RLM_MODULE_REJECT;
			}
			$p->{'reply'}{'Vendor-Specific'}{'Cisco'}{'h323-credit-amount'}[0] = "100";
			$p->{'reply'}{'Filter-Id'}[0] = 'Hello '.$p->{'request'}{'Net'}{'Src'}{'IP'}[0].' '.$p->{'request'}{'Vendor-Specific'}{'3GPP2'}{'Remote-IP'}{1}{'Address'}[0];
			$p->{'request'}{'User-Name'}[0] = 'tim';
			$p->{'control'}{'NAS-Identifier'}[0] = 'dummy';
		}
		return RLM_MODULE_OK;
	}
}

sub array_ops {
	my $p = shift();

	if ($#{$p->{'request'}{'Vendor-Specific'}{'Cisco'}{'AVPair'}} != 1) {
		freeradius::log(L_ERR, 'Incorrect $# value');
		return RLM_MODULE_REJECT;
	}

	if (!exists $p->{'request'}{'Vendor-Specific'}{'Cisco'}{'AVPair'}[1]) {
		freeradius::log(L_ERR, 'Failed to find request.Vendor-Specific.Cisco.AVPair[1]');
		return RLM_MODULE_REJECT;
	}

	if (defined $p->{'request'}{'Vendor-Specific'}{'Cisco'}{'AVPair'}[2]) {
		freeradius::log(L_ERR, 'Found request.Vendor-Specific.Cisco.AVPair[2]');
		return RLM_MODULE_REJECT;
	}

	push(@{$p->{'reply'}{'Reply-Message'}}, ('Hello', 'There'));
	unshift(@{$p->{'reply'}{'Reply-Message'}}, 'Firstly');

	my $mac = pop(@{$p->{'request'}{'Calling-Station-Id'}});
	if ($mac ne 'aa:bb:cc:dd:ee:ff') {
		freeradius::log(L_ERR, 'Incorrect Calling-Station-Id: ' . $mac);
		return RLM_MODULE_REJECT;
	}

	my $cisco = pop(@{$p->{'request'}{'Vendor-Specific'}{'Cisco'}{'AVPair'}});

	if ($cisco ne 'is=crazy') {
		freeradius::log(L_ERR, 'Invalid value for last Cisco.AVPair: ' . $cisco);
		return RLM_MODULE_REJECT;
	}

	my $filter = shift(@{$p->{'request'}{'Filter-Id'}});

	if ($filter ne 'Initial') {
		freeradius::log(L_ERR, 'Invalid value for first Filter: ' . $filter);
		return RLM_MODULE_REJECT;
	}

	return RLM_MODULE_OK;
}

sub hash_ops {
	my $p = shift();

	log_request_attributes($p);

	if (!exists $p->{'request'}{'Vendor-Specific'}) {
		freeradius::log(L_ERR, 'Couldn\'t find Vendor-Specific');
		return RLM_MODULE_REJECT;
	}

	$p->{'request'}{'Vendor-Specific'}{'3GPP2'}{'Remote-IP'}{2}{'Address'}[0] = '10.0.0.12';

	delete $p->{'request'}{'Vendor-Specific'}{'Cisco'};

	return RLM_MODULE_OK;
}

sub set_on_hash {
	my $p = shift();
	$p->{'reply'}{'User-Name'} = 'bob';
}

sub set_beyond_limit {
	my $p = shift();
	$p->{'reply'}{'Reply-Message'}[3] = 'Will set';
	$p->{'reply'}{'Reply-Message'}[10] = 'Will not set';
}

sub invalid_attr {
	my $p = shift();
	$p->{'reply'}{'Invalid-Attr'}[0] = 'Hello';
}

sub log_attributes {
	my %hash = %{$_[0]};
	my $indent = $_[1];
	for (keys %hash) {
		if (ref $hash{$_} eq 'HASH') {
			freeradius::log(L_DBG, ' 'x$indent . "$_ =>");
			log_attributes($hash{$_}, $indent + 2);
		} elsif (ref $hash{$_} eq 'ARRAY') {
			foreach my $attr (@{$hash{$_}}) {
				if (ref $attr eq 'HASH') {
					freeradius::log(L_DBG, ' 'x$indent . "$_ =>");
					log_attributes($attr, $indent + 2);
				} else {
					freeradius::log(L_DBG, ' 'x$indent . "$_ = $attr");
				}
			}
		}
	}
}

sub log_request_attributes {
	# This shouldn't be done in production environments!
	# This is only meant for debugging!
	my $p = shift();
	freeradius::log(L_DBG, "request:");
	log_attributes(\%{$p->{'request'}}, 2);
}
