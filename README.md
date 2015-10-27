[![Build Status](https://travis-ci.org/farrokhi/ifstatd.svg)](https://travis-ci.org/farrokhi/ifstatd)

# ifstatd_ plugin

Interface statistics plugin for munin with [supersampling](http://guide.munin-monitoring.org/en/latest/plugin/supersampling.html) capability

NOTE: This plugin builds on FreeBSD and Darwin, but not tested on other paltforms.

NOTE: This program depends on [libpidutil](https://github.com/farrokhi/libpidutil)

## Installation
1. Clone this repo and build using `make`
2. Copy `ifstatd_` binary to munin plugins directory:

	```
	cp ifstatd_ /usr/local/share/munin/plugins/
	```
3. Make necessary links to active plugins directory :

	```
	ln -s /usr/local/share/munin/ifstatd _/usr/local/etc/munin/plugins/ifstatd_em0
	```
4. Make sure plugin is run as `root` by adding these lines to munin
`plugins.conf`:

	```
	[ifstatd_*]
	user root
	```
5. Start `ifstatd_` as daemon:

	```
	munin-run ifstatd_em0 acquire
	```
6. Restart `munin-node` service:

	```
	service munin-node restart
	```

