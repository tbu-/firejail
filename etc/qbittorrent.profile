# abittorrent profile
include /etc/firejail/disable-mgmt.inc
include /etc/firejail/disable-secret.inc
blacklist ${HOME}/.adobe
blacklist ${HOME}/.macromedia
blacklist ${HOME}/.mozilla
blacklist ${HOME}/.icedove
blacklist ${HOME}/.thunderbird
caps.drop all
seccomp
netfilter

