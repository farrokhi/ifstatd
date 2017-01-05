FreeBSD rc script lives here. It starts ifstatd daemon for interfaces, based on your ifstatd symlinks in munin plugins subdirectory.
It should be copied to `/usr/local/etc/rc.d/` and following variable should be added to `/etc/rc.conf`:
```
ifstatd_enable="YES"
```
You can manually start it by issuing following command:
```
service ifstatd start
```

