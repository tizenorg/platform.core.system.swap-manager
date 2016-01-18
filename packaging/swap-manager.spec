Name:       swap-manager
Summary:    SWAP manager
Version:    3.0
Release:    1
Group:      System/Libraries
License:    Apache-2.0
Source:    %{name}_%{version}.tar.gz
BuildRequires:  smack-devel
BuildRequires:  libattr-devel
BuildRequires:  glib2-devel
BuildRequires:  aul-devel
BuildRequires:  vconf-devel
BuildRequires:  capi-system-info-devel
BuildRequires:  capi-system-runtime-info-devel
BuildRequires:  pkgconfig(ecore)
%if "%_project" != "Kirana_SWA_OPEN:Build" && "%_project" != "Kirana_SWA_OPEN:Daily"
Requires:  swap-modules
%endif
Requires:  swap-probe
Requires:  sdbd

%ifarch %{arm}
BuildRequires: systemd-devel
%endif

%description
SWAP manager is a part of data collection back-end for DA.
This binary will be installed in target.

%prep
%setup -q -n %{name}_%{version}

%build
cd daemon
make

%install
rm -rf ${RPM_BUILD_ROOT}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}

#systemd
mkdir -p %{buildroot}%{_libdir}/systemd/system

%ifarch %{arm}
install -m 0644 swap.service %{buildroot}%{_libdir}/systemd/system/swap.service
install -m 0644 swap.init.service %{buildroot}%{_libdir}/systemd/system/swap.init.service
install -m 0644 swap.socket %{buildroot}%{_libdir}/systemd/system/swap.socket

mkdir -p %{buildroot}/%{_libdir}/systemd/system/sockets.target.wants/
mkdir -p %{buildroot}/%{_libdir}/systemd/system/sockets.target.wants

ln -s %{_libdir}/systemd/system/swap.init.service %{buildroot}/%{_libdir}/systemd/system/sockets.target.wants/
ln -s %{_libdir}/systemd/system/swap.socket %{buildroot}/%{_libdir}/systemd/system/sockets.target.wants/
%endif

cd daemon
%make_install

%files
#systemd
%ifarch %{arm}
%{_libdir}/systemd/system/swap.init.service
%{_libdir}/systemd/system/swap.service
%{_libdir}/systemd/system/swap.socket

%{_libdir}/systemd/system/sockets.target.wants/swap.init.service
%{_libdir}/systemd/system/sockets.target.wants/swap.socket
%endif

/usr/share/license/%{name}
%manifest swap-manager.manifest

%ifarch %{arm}
%defattr(-,root,root,-)
/opt/swap/sdk/service_init.sh
/opt/swap/sdk/service_preinit.sh

#%defattr(-,developer,developer,-)
%defattr(-,system,system,-)
%else

%defattr(-,root,root,-)
%endif

%{_prefix}/bin/da_manager
%{_prefix}/bin/swap-start.sh
%{_prefix}/bin/swap-stop.sh

%changelog

