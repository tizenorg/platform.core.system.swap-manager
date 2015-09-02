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

mkdir -p %{buildroot}%{_libdir}/systemd/system

%ifarch %{ix86}
install -m 0644 swap.service %{buildroot}%{_libdir}/systemd/system/swap.service
%else
install -m 0644 swap.service %{buildroot}%{_libdir}/systemd/system/swap.service
%endif

cd daemon
%make_install

%files
%{_libdir}/systemd/system/swap.service

/usr/share/license/%{name}
%manifest swap-manager.manifest
%defattr(-,root,root,-)
%{_prefix}/bin/da_manager
/opt/swap/sdk/start.sh
/opt/swap/sdk/stop.sh

%changelog

