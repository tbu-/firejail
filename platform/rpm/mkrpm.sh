#!/bin/bash
VERSION="0.9.14"
rm -fr ~/rpmbuild
rm -f firejail-$VERSION-1.x86_64.rpm

mkdir -p ~/rpmbuild/{RPMS,SRPMS,BUILD,SOURCES,SPECS,tmp}
cat <<EOF >~/.rpmmacros
%_topdir   %(echo $HOME)/rpmbuild
%_tmppath  %{_topdir}/tmp
EOF

cd ~/rpmbuild
echo "building directory tree"

mkdir -p firejail-$VERSION/usr/bin
install -m 755 /usr/bin/firejail firejail-$VERSION/usr/bin/.
install -m 755 /usr/bin/firemon firejail-$VERSION/usr/bin/.

mkdir -p  firejail-$VERSION/usr/lib/firejail
install -m 644 /usr/lib/firejail/libtrace.so  firejail-$VERSION/usr/lib/firejail/.

mkdir -p firejail-$VERSION/usr/share/man/man1
install -m 644 /usr/share/man/man1/firejail.1.gz firejail-$VERSION/usr/share/man/man1/.
install -m 644 /usr/share/man/man1/firemon.1.gz firejail-$VERSION/usr/share/man/man1/.

mkdir -p firejail-$VERSION/usr/share/doc/packages/firejail
install -m 644 /usr/share/doc/firejail/COPYING firejail-$VERSION/usr/share/doc/packages/firejail/.
install -m 644 /usr/share/doc/firejail/README firejail-$VERSION/usr/share/doc/packages/firejail/.
install -m 644 /usr/share/doc/firejail/RELNOTES firejail-$VERSION/usr/share/doc/packages/firejail/.

mkdir -p firejail-$VERSION/etc/firejail
install -m 644 /etc/firejail/firefox.profile firejail-$VERSION/etc/firejail/firefox.profile
install -m 644 /etc/firejail/evince.profile firejail-$VERSION/etc/firejail/evince.profile
install -m 644 /etc/firejail/midori.profile firejail-$VERSION/etc/firejail/midori.profile
install -m 644 /etc/firejail/login.users firejail-$VERSION/etc/firejail/login.users
install -m 644 /etc/firejail/chromium.profile firejail-$VERSION/etc/firejail/chromium.profile
install -m 644 /etc/firejail/chromium-browser.profile firejail-$VERSION/etc/firejail/chromium-browser.profile


mkdir -p firejail-$VERSION/usr/share/bash-completion/completions
install -m 644 /usr/share/bash-completion/completions/firejail  firejail-$VERSION/usr/share/bash-completion/completions/.

echo "building tar.gz archive"
tar -czvf firejail-$VERSION.tar.gz firejail-$VERSION

cp firejail-$VERSION.tar.gz SOURCES/.

echo "building config spec"
cat <<EOF > SPECS/firejail.spec
%define        __spec_install_post %{nil}
%define          debug_package %{nil}
%define        __os_install_post %{_dbpath}/brp-compress

Summary: Linux namepaces sandbox program
Name: firejail
Version: $VERSION
Release: 1
License: GPL+
Group: Development/Tools
SOURCE0 : %{name}-%{version}.tar.gz
URL: http://firejail.sourceforege.net

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
Firejail  is  a  SUID sandbox program that reduces the risk of security
breaches by restricting the running environment of untrusted applications
using Linux namespaces. It includes a sandbox profile for Mozilla Firefox.

%prep
%setup -q

%build

%install
rm -rf %{buildroot}
mkdir -p  %{buildroot}

cp -a * %{buildroot}


%clean
rm -rf %{buildroot}


%files
%defattr(-,root,root,-)
%config(noreplace) %{_sysconfdir}/%{name}/firefox.profile
%config(noreplace) %{_sysconfdir}/%{name}/midori.profile
%config(noreplace) %{_sysconfdir}/%{name}/evince.profile
%config(noreplace) %{_sysconfdir}/%{name}/chromium.profile
%config(noreplace) %{_sysconfdir}/%{name}/chromium-browser.profile
%config(noreplace) %{_sysconfdir}/%{name}/login.users
/usr/bin/firejail
/usr/bin/firemon
/usr/lib/firejail/libtrace.so
/usr/share/doc/packages/firejail/COPYING
/usr/share/doc/packages/firejail/README
/usr/share/doc/packages/firejail/RELNOTES
/usr/share/man/man1/firejail.1.gz
/usr/share/man/man1/firemon.1.gz
/usr/share/bash-completion/completions/firejail
 
%post
chmod u+s /usr/bin/firejail

%changelog
* Wed Oct 8 2014  netblue30 <netblue30@yahoo.com> 0.9.14-1
 - Added support for user defined seccomp blacklists
 - Added syscall trace support
 - Added --tmpfs option
 - Added --balcklist option
 - Added --read-only option
 - Added --bind option
 - Logging enhancements
 - --overlay option was reactivated
 - Added firemon support to print the ARP table for each sandbox
 - Added firemon support to print the route table for each sandbox
 - Added firemon support to print interface information for each sandbox
 - bugfixes

* Tue Sep 16 2014 netblue30 <netblue30@yahoo.com> 0.9.12-1
 - Added capabilities support
 - Added support for CentOS 7
 - bugfixes

EOF

echo "building rpm"
rpmbuild -ba SPECS/firejail.spec
rpm -qpl RPMS/x86_64/firejail-$VERSION-1.x86_64.rpm
cd ..
rm -f firejail-$VERSION-1.x86_64.rpm
cp rpmbuild/RPMS/x86_64/firejail-$VERSION-1.x86_64.rpm .

