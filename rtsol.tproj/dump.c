/*
 * Copyright (c) 2009 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

/*	$KAME: dump.c,v 1.8 2000/10/05 22:20:39 itojun Exp $	*/

/*
 * Copyright (C) 1999 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/usr.sbin/rtsold/dump.c,v 1.1.2.3 2001/07/03 11:02:16 ume Exp $
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/icmp6.h>

#include <syslog.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "rtsold.h"

static FILE *fp;

extern struct ifinfo *iflist;

static void dump_interface_status __P((void));
static char *sec2str __P((time_t));
char *ifstatstr[] = {
	"IDLE", "DELAY", "PROBE", "DOWN", "TENTATIVE", "OPTIMISTIC"
};

static void
dump_interface_status()
{
	struct ifinfo *ifinfo;
	struct timeval now;

	gettimeofday(&now, NULL);

	for (ifinfo = iflist; ifinfo; ifinfo = ifinfo->next) {
		fprintf(fp, "Interface %s\n", ifinfo->ifname);
		fprintf(fp, "  probe interval: ");
		if (ifinfo->probeinterval) {
			fprintf(fp, "%d\n", ifinfo->probeinterval);
			fprintf(fp, "  probe timer: %d\n", ifinfo->probetimer);
		}
		else {
			fprintf(fp, "infinity\n");
			fprintf(fp, "  no probe timer\n");
		}
		fprintf(fp, "  interface status: %s\n",
			ifinfo->active > 0 ? "active" : "inactive");
		fprintf(fp, "  rtsold status: %s\n", ifstatstr[ifinfo->state]);
		fprintf(fp, "  carrier detection: %s\n",
			ifinfo->mediareqok ? "available" : "unavailable");
		fprintf(fp, "  probes: %d, dadcount = %d\n",
			ifinfo->probes, ifinfo->dadcount);
		if (ifinfo->timer.tv_sec == tm_max.tv_sec &&
		    ifinfo->timer.tv_usec == tm_max.tv_usec)
			fprintf(fp, "  no timer\n");
		else {
			fprintf(fp, "  timer: interval=%d:%d, expire=%s\n",
				(int)ifinfo->timer.tv_sec,
				(int)ifinfo->timer.tv_usec,
				(ifinfo->expire.tv_sec < now.tv_sec) ? "expired"
				: sec2str(ifinfo->expire.tv_sec - now.tv_sec));
		}
		fprintf(fp, "  number of valid RAs: %d\n", ifinfo->racnt);
	}
}

void
rtsold_dump_file(dumpfile)
	char *dumpfile;
{
	if ((fp = fopen(dumpfile, "w")) == NULL) {
		warnmsg(LOG_WARNING, __FUNCTION__, "open a dump file(%s): %s",
			dumpfile, strerror(errno));
		return;
	}

	dump_interface_status();

	fclose(fp);
}

static char *
sec2str(total)
	time_t total;
{
	static char result[256];
	int days, hours, mins, secs;
	int first = 1;
	char *p = result;

	days = total / 3600 / 24;
	hours = (total / 3600) % 24;
	mins = (total / 60) % 60;
	secs = total % 60;

	if (days) {
		first = 0;
		p += snprintf(p, sizeof(result) - (p - result), "%dd", days);
	}
	if (!first || hours) {
		first = 0;
		p += snprintf(p, sizeof(result) - (p - result), "%dh", hours);
	}
	if (!first || mins) {
		first = 0;
		p += snprintf(p, sizeof(result) - (p - result), "%dm", mins);
	}
	snprintf(p, sizeof(result) - (p - result), "%ds", secs);

	return(result);
}
