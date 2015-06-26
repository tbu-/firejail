# Firejail profile for Mozilla Firefox (Iceweasel in Debian)
include /etc/firejail/disable-mgmt.inc
include /etc/firejail/disable-secret.inc
blacklist ${HOME}/.adobe
blacklist ${HOME}/.macromedia
blacklist ${HOME}/.icedove
blacklist ${HOME}/.thunderbird
blacklist ${HOME}/.config/opera
blacklist ${HOME}/.config/chromium
blacklist ${HOME}/.config/google-chrome
caps.drop all
seccomp
netfilter
noroot

