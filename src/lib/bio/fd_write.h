/*
 *	Common finalization code for the write functions.
 *
 *	This is in a header file because of "goto retry" in fd_errno.h.
 *
 *	@todo - do we want the callbacks to notify the _previous_ BIO in the chain?  That way the top-level
 *	BIO can notify the application.
 */
if (rcode > 0) {
	/*
	 *	We weren't blocked, but we are now.
	 */
	if (!my->info.write_blocked) {
		if ((size_t) rcode == size) {
			return rcode;
		}

		fr_assert((size_t) rcode < size);

		/*
		 *	Set the flag and run the callback.
		 */
		my->info.write_blocked = true;
		if (my->cb.write_blocked) my->cb.write_blocked((fr_bio_t *) my);

		return rcode;	
	}

	/*
	 *	We were blocked.  We're still blocked if we wrote _less_ than the amount of requested data.
	 *	If we wrote all of the data which was requested, then we're unblocked.
	 */
	my->info.write_blocked = ((size_t) rcode == size);

	/*
	 *	Call the "resume" function if we transitioned to being unblocked.
	 */
	if (!my->info.write_blocked && my->cb.write_resume) my->cb.write_resume((fr_bio_t *) my);

	return rcode;
}

if (rcode == 0) return rcode;

#undef flag_blocked
#define flag_blocked write_blocked
#include "fd_errno.h"
