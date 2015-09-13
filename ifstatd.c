/*
Copyright (c) 2015, Babak Farrokhi
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_types.h>
#include <ifaddrs.h>

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

#define IFA_STAT(s)     (((struct if_data *)ifa->ifa_data)->ifi_ ## s)
#define RESOLUTION	10

struct iftot {
        u_long  ift_ip;                 /* input packets */
        u_long  ift_ie;                 /* input errors */
        u_long  ift_id;                 /* input drops */
        u_long  ift_op;                 /* output packets */
        u_long  ift_oe;                 /* output errors */
        u_long  ift_od;                 /* output drops */
        u_long  ift_co;                 /* collisions */
        u_long  ift_ib;                 /* input bytes */
        u_long  ift_ob;                 /* output bytes */
};

/* Globals */
char *program_name = "ifstatd_";
char *interface;
char *pid_filename;
char *cache_filename;

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
                /*st->ift_ie += IFA_STAT(ierrors);*/
                /*st->ift_id += IFA_STAT(iqdrops);*/
                st->ift_ib += IFA_STAT(ibytes);
                st->ift_op += IFA_STAT(opackets);
                /*st->ift_oe += IFA_STAT(oerrors);*/
                /* st->ift_od += IFA_STAT(oqdrops);*/
                st->ift_ob += IFA_STAT(obytes);
                /*st->ift_co += IFA_STAT(collisions);*/
        }

        if (interface && found == false)
                err(EX_DATAERR, "interface %s not found", interface);

        freeifaddrs(ifap);
}

int config(char *iface)
{
	printf(
        "graph_order rbytes obytes\n"
		"graph_title %s Interface (HighRes)\n"
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
		,iface,RESOLUTION,RESOLUTION
	);

	return(0);
}

time_t wait_for(int seconds) {

	struct timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp);

	time_t current_epoch = tp.tv_sec;
	long nsec_to_sleep = 1000*1000*1000 - tp.tv_nsec;

	/* Only sleep if needed */
	if (nsec_to_sleep > 0) {
		tp.tv_sec = seconds - 1;
		tp.tv_nsec = nsec_to_sleep;
		nanosleep(&tp, NULL);
	}

	return current_epoch + seconds;
}

int acquire()
{
	struct iftot ift, *tot;
	time_t epoch;

	tot = &ift;

	/* fork ourselves if not asked otherwise */
	char* no_fork = getenv("no_fork");
	if (! no_fork || strcmp("1", no_fork)) {
		if (fork()) return(0);
		/* we are the child, complete the daemonization */

		/* Close standard IO */
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);

		/* create new session and process group */
		setsid();
	}

	/* persist pid */
	FILE *pid_file = fopen(pid_filename, "w");
	fprintf(pid_file, "%d\n", getpid());
	fclose(pid_file);

	FILE *cache_file = fopen(cache_filename, "a");


	/* looping to collect traffic stat every RESOLUTION seconds */
	
	while(1) {

		epoch=wait_for(RESOLUTION);

		flock(fileno(cache_file), LOCK_EX);

		fill_iftot(tot);
		fprintf(cache_file, "obytes.value %ld:%lu\nrbytes.value %ld:%lu\n", epoch, tot->ift_ob, epoch, tot->ift_ib);
		fflush(cache_file);

		flock(fileno(cache_file), LOCK_UN);
	}	

	fclose(cache_file);
	return(0);
}

int fetch()
{
	/* this should return data from cache file */
	FILE* cache_file = fopen(cache_filename, "r+");

	/* lock */
	flock(fileno(cache_file), LOCK_EX);

	/* cat the cache_file to stdout */
	char buffer[1024];
	while (fgets(buffer, 1024, cache_file)) {
		printf("%s", buffer);
	}

	ftruncate(fileno(cache_file), 0);
	fclose(cache_file);

 	return(0);
}

int 
main(int argc, char* argv[]) 
{
	if (argv[0] && argv[0][0])
		program_name = argv[0];

	/* figure out program name */
	while (strchr(program_name, '/')) {
		program_name=strchr(program_name, '/')+1;
	}
	
	/* extract interface name from plugin name */
	if (strchr(program_name,'_')) {
		interface=strchr(program_name,'_')+1;
	}

	/* program should always run with a valid
		executable name */
	if (strlen(interface) < 1) {
		errx(EX_USAGE, "Please run from symlink");
	}

	/* resolve paths */
	char *MUNIN_PLUGSTATE = getenv("MUNIN_PLUGSTATE");

	/* Default is current directory */
	if (!MUNIN_PLUGSTATE) MUNIN_PLUGSTATE = ".";

	asprintf(&pid_filename,   "%s/%s.pid",   MUNIN_PLUGSTATE, program_name);
	asprintf(&cache_filename, "%s/%s.value", MUNIN_PLUGSTATE, program_name);

	if (argc > 1) {
		char* first_arg = argv[1];
		if (! strcmp(first_arg, "config")) {
			return config(interface);
		}

		if (! strcmp(first_arg, "acquire")) {
			return acquire();
		}
	}

	return fetch();
}
