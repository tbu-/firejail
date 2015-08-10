Firejail is a SUID sandbox program that reduces the risk of security breaches by restricting the running environment of untrusted applications using Linux namespaces, seccomp-bpf and Linux capabilities. It allows a process and all its descendants to have their own private view of the globally shared kernel resources, such as the network stack, process table, mount table. Firejail can work in a SELinux or AppArmor environment, and it is integrated with Linux Control Groups.

Written in C with virtually no dependencies, the software runs on any Linux computer with a 3.x kernel version or newer. It can sandbox any type of processes: servers, graphical applications, and even user login sessions. The software includes sandbox profiles for a number of more common Linux programs, such as Mozilla Firefox, Chromium, VLC, Transmission etc.

The sandbox is lightweight, the overhead is low. There are no complicated configuration files to edit, no socket connections open, no daemons running in the background. All security features are implemented directly in Linux kernel and available on any Linux computer. To start the sandbox, prefix your command with “firejail”:

    $ firejail firefox            # starting Mozilla Firefox
    $ firejail transmission-gtk   # starting Transmission BitTorrent 
    $ firejail vlc                # starting VideoLAN Client
    $ sudo firejail "/etc/init.d/nginx start && sleep inf"

Project webpage: https://l3net.wordpress.com/projects/firejail/

Download and Installation: https://l3net.wordpress.com/projects/firejail/firejail-download-and-install/

Features: https://l3net.wordpress.com/projects/firejail/firejail-features/

Usage: https://l3net.wordpress.com/projects/firejail/firejail-usage/
