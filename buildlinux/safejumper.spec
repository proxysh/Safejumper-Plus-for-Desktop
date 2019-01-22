#
# spec file for package safejumper
#
# Copyright (c) 2016 Jeremy Whiting <jeremypwhiting@gmail.com>
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

Name:           safejumper
Summary:        VPN client for Safejumper network.
License:        GPL-2.0 and GPL-3.0
Group:          Productivity/Networking/Web/Utilities
Version:        2019.01.21
Release:        0
Url:            http://proxy.sh
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Source:         safejumper-%version.tar.gz
Conflicts:      safejumper < %version-%release
Requires:	net-tools

# Do not check any files in env for requires
# %global __requires_exclude_from ^/opt/safejumper/env/.*$
# %global __requires_exclude ^/opt/safejumper/env/.*$

%description
VPN client for Proxy.sh network.
Safejumper is a lightweight OpenVPN client specifically designed for the Proxy.sh VPN network.

Authors:
--------
    Jeremy Whiting <info@3monkeysinternational.com>
#--------------------------------------------------------------------------------
%prep
%setup -q 

%build

%install
#
# First install all dist files
#
mkdir -p $RPM_BUILD_ROOT/opt/safejumper/
mkdir -p $RPM_BUILD_ROOT/usr/share/applications/
mkdir -p $RPM_BUILD_ROOT/usr/share/icons/hicolor/16x16/apps
mkdir -p $RPM_BUILD_ROOT/usr/share/icons/hicolor/32x32/apps
mkdir -p $RPM_BUILD_ROOT/usr/share/icons/hicolor/64x64/apps
mkdir -p $RPM_BUILD_ROOT/usr/lib/systemd/system/
install -m 0755 safejumper                 $RPM_BUILD_ROOT/opt/safejumper/safejumper
install -m 0755 safejumperservice          $RPM_BUILD_ROOT/opt/safejumper/safejumperservice
install -m 0755 netdown                   $RPM_BUILD_ROOT/opt/safejumper/netdown
install -m 0755 openvpn                   $RPM_BUILD_ROOT/opt/safejumper/openvpn
install -m 0744 client.down.sh            $RPM_BUILD_ROOT/opt/safejumper/client.down.sh
install -m 0744 client.up.sh              $RPM_BUILD_ROOT/opt/safejumper/client.up.sh
install -m 0744 update-systemd-resolved   $RPM_BUILD_ROOT/opt/safejumper/update-systemd-resolved
install -m 0744 detectresolve.sh          $RPM_BUILD_ROOT/opt/safejumper/detectresolve.sh
install -m 0755 safejumper.desktop         $RPM_BUILD_ROOT/usr/share/applications
install -m 0644 safejumper.service         $RPM_BUILD_ROOT/usr/lib/systemd/system/safejumper.service
install -m 0744 icons/16x16/apps/safejumper.png     $RPM_BUILD_ROOT/usr/share/icons/hicolor/16x16/apps
install -m 0744 icons/32x32/apps/safejumper.png     $RPM_BUILD_ROOT/usr/share/icons/hicolor/32x32/apps
install -m 0744 icons/64x64/apps/safejumper.png     $RPM_BUILD_ROOT/usr/share/icons/hicolor/64x64/apps

%pre

%preun
systemctl disable safejumper
systemctl stop safejumper

%post
systemctl enable safejumper
systemctl start safejumper

%posttrans

%postun

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc {README,LICENSE}
%dir /opt/safejumper/
/opt/safejumper/safejumper
/opt/safejumper/safejumperservice
/opt/safejumper/netdown
/opt/safejumper/openvpn
/opt/safejumper/client.down.sh
/opt/safejumper/client.up.sh
/opt/safejumper/update-systemd-resolved
/opt/safejumper/detectresolve.sh
/usr/share/applications/safejumper.desktop
/usr/share/icons/hicolor/16x16/apps/safejumper.png
/usr/share/icons/hicolor/32x32/apps/safejumper.png
/usr/share/icons/hicolor/64x64/apps/safejumper.png
/usr/lib/systemd/system/safejumper.service

%changelog
