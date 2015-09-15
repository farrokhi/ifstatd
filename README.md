# ifstatd_ plugin
[![Issues](https://img.shields.io/github/issues/farrokhi/ifstatd.svg)](https://github.com/farrokhi/ifstatd/issues)
[![Coverity Scan](https://img.shields.io/coverity/scan/6341.svg)](https://scan.coverity.com/projects/farrokhi-ifstatd)
[![GitHub License](https://img.shields.io/github/license/farrokhi/ifstatd.svg)](https://github.com/farrokhi/ifstatd/blob/master/LICENSE)

Interface statistics plugin for munin with [supersampling](http://guide.munin-monitoring.org/en/latest/plugin/supersampling.html) capability

NOTE: This plugin created for FreeBSD and might not run on other OSes

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

