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

Name:           safejumperplus
Summary:        VPN client for Safejumper Plus network.
License:        GPL-2.0 and GPL-3.0
Group:          Productivity/Networking/Web/Utilities
Version:        2019.04.24
Release:        0
Url:            http://proxy.sh
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Source:         safejumperplus-%version.tar.gz
Conflicts:      safejumperplus < %version-%release
Requires:	net-tools

# Do not check any files in env for requires
# %global __requires_exclude_from ^/opt/safejumperplus/env/.*$
# %global __requires_exclude ^/opt/safejumperplus/env/.*$

%description
VPN client for Proxy.sh network.
Safejumper Plus is a lightweight OpenVPN client specifically designed for the Proxy.sh VPN network.

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
mkdir -p $RPM_BUILD_ROOT/opt/safejumperplus/
mkdir -p $RPM_BUILD_ROOT/usr/share/applications/
mkdir -p $RPM_BUILD_ROOT/usr/share/icons/hicolor/16x16/apps
mkdir -p $RPM_BUILD_ROOT/usr/share/icons/hicolor/32x32/apps
mkdir -p $RPM_BUILD_ROOT/usr/share/icons/hicolor/64x64/apps
mkdir -p $RPM_BUILD_ROOT/usr/lib/systemd/system/
install -m 0755 safejumperplus                 $RPM_BUILD_ROOT/opt/safejumperplus/safejumperplus
install -m 0755 safejumperplusservice          $RPM_BUILD_ROOT/opt/safejumperplus/safejumperplusservice
install -m 0755 netdown                   $RPM_BUILD_ROOT/opt/safejumperplus/netdown
install -m 0755 openvpn                   $RPM_BUILD_ROOT/opt/safejumperplus/openvpn
install -m 0744 client.down.sh            $RPM_BUILD_ROOT/opt/safejumperplus/client.down.sh
install -m 0744 client.up.sh              $RPM_BUILD_ROOT/opt/safejumperplus/client.up.sh
install -m 0744 update-systemd-resolved   $RPM_BUILD_ROOT/opt/safejumperplus/update-systemd-resolved
install -m 0744 detectresolve.sh          $RPM_BUILD_ROOT/opt/safejumperplus/detectresolve.sh
install -m 0755 safejumperplus.desktop         $RPM_BUILD_ROOT/usr/share/applications
install -m 0644 safejumperplus.service         $RPM_BUILD_ROOT/usr/lib/systemd/system/safejumperplus.service
install -m 0744 icons/16x16/apps/safejumperplus.png     $RPM_BUILD_ROOT/usr/share/icons/hicolor/16x16/apps
install -m 0744 icons/32x32/apps/safejumperplus.png     $RPM_BUILD_ROOT/usr/share/icons/hicolor/32x32/apps
install -m 0744 icons/64x64/apps/safejumperplus.png     $RPM_BUILD_ROOT/usr/share/icons/hicolor/64x64/apps

%pre

%preun
systemctl disable safejumperplus
systemctl stop safejumperplus

%post
systemctl enable safejumperplus
systemctl start safejumperplus

%posttrans

%postun

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc {README,LICENSE}
%dir /opt/safejumperplus/
/opt/safejumperplus/safejumperplus
/opt/safejumperplus/safejumperplusservice
/opt/safejumperplus/netdown
/opt/safejumperplus/openvpn
/opt/safejumperplus/client.down.sh
/opt/safejumperplus/client.up.sh
/opt/safejumperplus/update-systemd-resolved
/opt/safejumperplus/detectresolve.sh
/usr/share/applications/safejumperplus.desktop
/usr/share/icons/hicolor/16x16/apps/safejumperplus.png
/usr/share/icons/hicolor/32x32/apps/safejumperplus.png
/usr/share/icons/hicolor/64x64/apps/safejumperplus.png
/usr/lib/systemd/system/safejumperplus.service

%changelog
