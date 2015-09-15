# ifstatd
[![Issues](https://img.shields.io/github/issues/farrokhi/ifstatd.svg)](https://github.com/farrokhi/ifstatd/issues)
[![Coverity Scan](https://img.shields.io/coverity/scan/6341.svg)](https://scan.coverity.com/projects/farrokhi-ifstatd)
[![GitHub License](https://img.shields.io/github/license/farrokhi/ifstatd.svg)](https://github.com/farrokhi/ifstatd/blob/master/LICENSE)

interface statistics plugin for munin with supersampling capability

## Installation
1. Clone this repo and build using `make`
2. Copy `ifstatd_` binary to munin plugins directory 
	(e.g. `cp ifstatd_
/usr/local/share/munin/plugins/`)
3. Make necessary links to active plugins directory 
	(e.g. `ln -s /usr/local/share/munin/ifstatd_
	/usr/local/etc/munin/plugins/ifstatd_em0`)
4. Restart munin-agent	
