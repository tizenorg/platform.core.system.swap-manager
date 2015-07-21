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
BuildRequires:  libwebsockets-devel
BuildRequires:  pkgconfig(json-c)
BuildRequires:  pkgconfig(ecore)
%if "%{?tizen_profile_name}" == "mobile"
BuildRequires:  pkgconfig(callmgr_client)
%endif
BuildRequires:  swap-probe-devel
%if "%{?tizen_profile_name}" == "tv"
BuildRequires:  webkit2-efl-tv
BuildRequires:  webkit2-efl-tv-debuginfo
%else
BuildRequires:  webkit2-efl
BuildRequires:  webkit2-efl-debuginfo
%endif
BuildRequires:  launchpad
BuildRequires:  app-core-efl
BuildRequires:  app-core-debuginfo
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
pushd scripts
echo "__tizen_profile_name__="%{?tizen_profile_name} > dyn_vars
popd
cd daemon

SWAP_BUILD_CONF=""
%if "%{?tizen_profile_name}" == "mobile"
SWAP_BUILD_CONF=$SWAP_BUILD_CONF CALL_MNGR=y
%endif

%if "%{?tizen_profile_name}" != "tv"
SWAP_BUILD_CONF=$SWAP_BUILD_CONF WSP_SUPPORT=y
%endif

$SWAP_BUILD_CONF make
#CALL_MNGR=y WSP_SUPPORT=y make

%install
rm -rf ${RPM_BUILD_ROOT}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}
cd daemon
%make_install

%files
/usr/share/license/%{name}
%manifest swap-manager.manifest
%defattr(-,root,root,-)
%{_prefix}/bin/da_manager
/opt/swap/sdk/start.sh
/opt/swap/sdk/stop.sh
/opt/swap/sdk/init_preload.sh

%changelog

