/*-
 * Copyright (c) 2015, Babak Farrokhi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <net/if.h>
#include <ifaddrs.h>

#ifdef __linux__
#    define AF_LINK AF_PACKET
#else
#include <net/if_var.h>
#include <net/if_types.h>
#endif

#include <err.h>
#include <errno.h>

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <time.h>
#include <pidutil.h>

#ifdef __MACH__
#include <sys/time.h>
#define CLOCK_REALTIME 0
#endif

#ifdef __linux__
/*
 *  * Structure describing information about an interface
 *   * which may be of interest to management entities.
 *    */
struct if_data {
        /* generic interface information */
        u_char  ifi_type;               /* ethernet, tokenring, etc */
        u_char  ifi_physical;           /* e.g., AUI, Thinnet, 10base-T, etc */
        u_char  ifi_addrlen;            /* media address length */
        u_char  ifi_hdrlen;             /* media header length */
        u_char  ifi_link_state;         /* current link state */
        u_char  ifi_vhid;               /* carp vhid */
        u_char  ifi_baudrate_pf;        /* baudrate power factor */
        u_char  ifi_datalen;            /* length of this data struct */
        u_long  ifi_mtu;                /* maximum transmission unit */
        u_long  ifi_metric;             /* routing metric (external only) */
        u_long  ifi_baudrate;           /* linespeed */
        /* volatile statistics */
        u_long  ifi_ipackets;           /* packets received on interface */
        u_long  ifi_ierrors;            /* input errors on interface */
        u_long  ifi_opackets;           /* packets sent on interface */
        u_long  ifi_oerrors;            /* output errors on interface */
        u_long  ifi_collisions;         /* collisions on csma interfaces */
        u_long  ifi_ibytes;             /* total number of octets received */
        u_long  ifi_obytes;             /* total number of octets sent */
        u_long  ifi_imcasts;            /* packets received via multicast */
        u_long  ifi_omcasts;            /* packets sent via multicast */
        u_long  ifi_iqdrops;            /* dropped on input, this interface */
        u_long  ifi_noproto;            /* destined for unsupported protocol */
        uint64_t ifi_hwassist;          /* HW offload capabilities, see IFCAP */
        time_t  ifi_epoch;              /* uptime at attach or stat reset */
        struct  timeval ifi_lastchange; /* time of last administrative change */
#ifdef _IFI_OQDROPS
        u_long  ifi_oqdrops;            /* dropped on output */
#endif /* _IFI_OQFROPS */
};
#endif /* __linux __ */

#define IFA_STAT(s)     (((struct if_data *)ifa->ifa_data)->ifi_ ## s)
#define RESOLUTION	10

struct iftot {
	u_long	ift_ip;			/* input packets */
	u_long	ift_ie;			/* input errors */
	u_long	ift_id;			/* input drops */
	u_long	ift_op;			/* output packets */
	u_long	ift_oe;			/* output errors */
	u_long	ift_od;			/* output drops */
	u_long	ift_co;			/* collisions */
	u_long	ift_ib;			/* input bytes */
	u_long	ift_ob;			/* output bytes */
};

/* Globals */
char   *program_name = "ifstatd_";
char   *interface;
char   *pid_filename;
char   *cache_filename;
struct pidfh *pfh;

#ifdef __MACH__
/* clock_gettime is not implemented on OSX */
int 
clock_gettime(int clk_id, struct timespec *t)
{
	struct timeval now;
	int rv = gettimeofday(&now, NULL);

	if (rv)
		return rv;
	t->tv_sec = now.tv_sec;
	t->tv_nsec = now.tv_usec * 1000;
	return 0;
}

#endif

/*
 * Prepare for a clean shutdown
 */
void
daemon_shutdown()
{
	pidfile_remove(pfh);
}

/*
 * Act upon receiving signals
 */
void
signal_handler(int sig)
{
	switch (sig) {

	case SIGHUP:
	case SIGINT:
	case SIGTERM:
		daemon_shutdown();
		exit(EXIT_SUCCESS);
		break;
	default:
		break;
	}
}

/*
 * Obtain stats for interface(s).
 */
static void
fill_iftot(struct iftot *st)
{
	struct ifaddrs *ifap, *ifa;
	bool found = false;

	if (getifaddrs(&ifap) != 0)
		err(EX_OSERR, "getifaddrs");

	bzero(st, sizeof(*st));

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family != AF_LINK)
			continue;
		if (interface) {
			if (strcmp(ifa->ifa_name, interface) == 0)
				found = true;
			else
				continue;
		}
		st->ift_ip += IFA_STAT(ipackets);
		st->ift_ib += IFA_STAT(ibytes);
		st->ift_op += IFA_STAT(opackets);
		st->ift_ob += IFA_STAT(obytes);
	}

	if (interface && found == false)
		err(EX_DATAERR, "interface %s not found", interface);

	freeifaddrs(ifap);
}

/*
 * Print out munin plugin configuration
 */
int
config(char *iface)
{
	printf(
	    "graph_order rbytes obytes\n"
	    "graph_title %s Interface (%d seconds sampling)\n"
	    "graph_category network\n"
	    "graph_vlabel bits per second\n"
	    "update_rate %d\n"
	    "graph_data_size custom 1d, %ds for 1w, 1m for 1t, 5m for 1y\n"
	    "rbytes.label received\n"
	    "rbytes.type DERIVE\n"
	    "rbytes.graph no\n"
	    "rbytes.cdef rbytes,8,*\n"
	    "rbytes.min 0\n"
	    "obytes.label bps\n"
	    "obytes.type DERIVE\n"
	    "obytes.negative rbytes\n"
	    "obytes.cdef obytes,8,*\n"
	    "obytes.min 0\n"
	    "obytes.draw AREA\n"
	    ,iface, RESOLUTION, RESOLUTION, RESOLUTION
	);

	return (0);
}

/*
 * Wait for a certain amount of time
 */
time_t
wait_for(int seconds)
{

	struct timespec tp;

	bzero(&tp, sizeof(tp));

	clock_gettime(CLOCK_REALTIME, &tp);

	time_t current_epoch = tp.tv_sec;
	long nsec_to_sleep = 1000 * 1000 * 1000 - tp.tv_nsec;

	/* Only sleep if needed */
	if (nsec_to_sleep > 0) {
		tp.tv_sec = seconds - 1;
		tp.tv_nsec = nsec_to_sleep;
		nanosleep(&tp, NULL);
	}
	return current_epoch + seconds;
}

/*
 * Daemonize and persist pid
 */
int
daemon_start()
{
	struct iftot ift, *tot;
	time_t epoch;
	struct sigaction sig_action;
	sigset_t sig_set;
	pid_t otherpid;
	int curPID;

	tot = &ift;

	char *no_fork = getenv("no_fork");

	if (!no_fork || strcmp("1", no_fork)) {

		/* Check if parent process id is set */
		if (getppid() == 1) {
			/* PPID exists, therefore we are already a daemon */
			return (EXIT_FAILURE);
		}
		/* Check if we can acquire the pid file */
		pfh = pidfile_open(pid_filename, 0600, &otherpid);

		if (pfh == NULL) {
			if (errno == EEXIST) {
				errx(EXIT_FAILURE, "Daemon already running, pid: %jd.", (intmax_t)otherpid);
			}
			warn("Cannot open or create pidfile: %s", pid_filename);
		}
		/* start daemonizing */
		curPID = fork();

		switch (curPID) {
		case 0:		/* This process is the child */
			break;
		case -1:		/* fork() failed, should exit */
			return (EXIT_FAILURE);
		default:		/* fork() successful, should exit */
			return (EXIT_SUCCESS);
		}

		/* we are the child, complete the daemonization */

		/* Close standard IO */
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);

		/* Block unnecessary signals */
		sigemptyset(&sig_set);
		sigaddset(&sig_set, SIGCHLD);	/* ignore child - i.e. we
						 * don't need to wait for it */
		sigaddset(&sig_set, SIGTSTP);	/* ignore Tty stop signals */
		sigaddset(&sig_set, SIGTTOU);	/* ignore Tty background
						 * writes */
		sigaddset(&sig_set, SIGTTIN);	/* ignore Tty background reads */
		sigprocmask(SIG_BLOCK, &sig_set, NULL);	/* Block the above
							 * specified signals */

		/* Catch necessary signals */
		sig_action.sa_handler = signal_handler;
		sigemptyset(&sig_action.sa_mask);
		sig_action.sa_flags = 0;

		sigaction(SIGTERM, &sig_action, NULL);
		sigaction(SIGHUP, &sig_action, NULL);
		sigaction(SIGINT, &sig_action, NULL);

		/* create new session and process group */
		if (setsid() < 0)
			return (EXIT_FAILURE);

		/* persist pid */
		pidfile_write(pfh);

	}
	FILE *cache_file = fopen(cache_filename, "a");

	/* looping to collect traffic stat every RESOLUTION seconds */
	while (1) {

		epoch = wait_for(RESOLUTION);

		flockfile(cache_file);

		fill_iftot(tot);
		fprintf(cache_file, "obytes.value %ld:%lu\nrbytes.value %ld:%lu\n", epoch, tot->ift_ob, epoch, tot->ift_ib);
		fflush(cache_file);

		funlockfile(cache_file);
	}

	fclose(cache_file);
	return (0);
}

int
fetch()
{
	/* this should return data from cache file */
	FILE *cache_file = fopen(cache_filename, "r+");

	/* lock */
	flockfile(cache_file);

	/* cat the cache_file to stdout */
	char buffer[1024];

	while (fgets(buffer, 1024, cache_file)) {
		printf("%s", buffer);
	}

	ftruncate(fileno(cache_file), 0);
	fclose(cache_file);

	return (0);
}

int
main(int argc, char *argv[])
{
	if (argv[0] && argv[0][0])
		program_name = argv[0];

	/* figure out program name */
	while (strchr(program_name, '/')) {
		program_name = strchr(program_name, '/') + 1;
	}

	/* extract interface name from plugin name */
	if (strchr(program_name, '_')) {
		interface = strchr(program_name, '_') + 1;
	}
	/*
	 * program should always run with a valid executable name
	 */
	if (strlen(interface) < 1) {
		errx(EX_USAGE, "Please run from symlink");
	}
	/* resolve paths */
	char *MUNIN_PLUGSTATE = getenv("MUNIN_PLUGSTATE");

	/* Default is current directory */
	if (!MUNIN_PLUGSTATE)
		MUNIN_PLUGSTATE = ".";

	asprintf(&pid_filename, "%s/%s.pid", MUNIN_PLUGSTATE, program_name);
	asprintf(&cache_filename, "%s/%s.value", MUNIN_PLUGSTATE, program_name);

	if (argc > 1) {
		char *first_arg = argv[1];

		if (!strcmp(first_arg, "config")) {
			return config(interface);
		}
		if (!strcmp(first_arg, "acquire")) {
			return daemon_start();
		}
	}
	return fetch();
}
