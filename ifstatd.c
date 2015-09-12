#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
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

// Globals
char *interface="en1";
char* pid_filename;
char* cache_filename;

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
                st->ift_ie += IFA_STAT(ierrors);
                // st->ift_id += IFA_STAT(iqdrops);
                st->ift_ib += IFA_STAT(ibytes);
                st->ift_op += IFA_STAT(opackets);
                st->ift_oe += IFA_STAT(oerrors);
                // st->ift_od += IFA_STAT(oqdrops);
                st->ift_ob += IFA_STAT(obytes);
                st->ift_co += IFA_STAT(collisions);
        }

        if (interface && found == false)
                err(EX_DATAERR, "interface %s not found", interface);

        freeifaddrs(ifap);
}

int config(char *iface)
{
	printf(
		"graph_title HighRes Interface Traffic Stat\n"
		"graph_category network::10sec\n"
		"graph_vlabel Traffic\n"
		"update_rate 10\n"
		"graph_data_size custom 1d, 10s for 1w, 1m for 1t, 5m for 1y\n"
		""
	);

	return(0);
}

int acquire()
{
	return(0);
}

int fetch()
{
	struct iftot ift, *tot;

	tot = &ift;

	fill_iftot(tot);
	fprintf(stdout, "%s\tibytes: %lu\tobytes: %lu\n", interface, tot->ift_ib, tot->ift_ob);
	fflush(stdout);

 	return(0);
}

int 
main(int argc, char* argv[]) 
{
		/* resolve paths */
	char *MUNIN_PLUGSTATE = getenv("MUNIN_PLUGSTATE");

	/* Default is current directory */
	if (! MUNIN_PLUGSTATE) MUNIN_PLUGSTATE = ".";

	size_t MUNIN_PLUGSTATE_length = strlen(MUNIN_PLUGSTATE);

	pid_filename = malloc(MUNIN_PLUGSTATE_length + strlen("/ifstatd.") + strlen("pid") + 1); pid_filename[0] = '\0';
	cache_filename = malloc(MUNIN_PLUGSTATE_length + strlen("/ifstatd.") + strlen("value") + 1); cache_filename[0] = '\0';

	strcat(pid_filename, MUNIN_PLUGSTATE);
	strcat(pid_filename, "/ifstatd.pid");

	strcat(cache_filename, MUNIN_PLUGSTATE);
	strcat(cache_filename, "/ifstatd.value");

	if (argc > 1) {
		char* first_arg = argv[1];
		if (! strcmp(first_arg, "config")) {
			return config();
		}

		if (! strcmp(first_arg, "acquire")) {
			return acquire();
		}
	}

	return fetch();
}
