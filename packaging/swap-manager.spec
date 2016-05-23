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
BuildRequires:  wrt
BuildRequires:  pkgconfig(dlog)
BuildRequires:  libdlog
%if "%{sec_product_feature_profile_wearable}" == "1"
BuildRequires:  libjson-devel
%else
BuildRequires:  pkgconfig(json-c)
%endif
BuildRequires:  pkgconfig(ecore)
%if "%{?tizen_profile_name}" == "mobile"
BuildRequires:  call-manager
BuildRequires:  libcall-manager-devel
%endif
BuildRequires:  swap-probe-devel
BuildRequires:  swap-probe-elf
%if "%{?tizen_profile_name}" == "tv"
BuildRequires:  webkit2-efl-tv
%if "%{TIZEN_PRODUCT_TV}" != "1"
BuildRequires:  webkit2-efl-tv-debuginfo
%endif
%else
BuildRequires:  webkit2-efl
BuildRequires:  webkit2-efl-debuginfo
%endif
%if "%{sec_product_feature_profile_wearable}" == "1"
BuildRequires:  launchpad-process-pool
BuildRequires:  launchpad-loader
%else
BuildRequires:  launchpad
%endif
BuildRequires:  app-core-efl
BuildRequires:  evas-devel
BuildRequires:  elementary-devel
BuildRequires:  libXext-devel
%if "%{TIZEN_PRODUCT_TV}" != "1"
BuildRequires:  app-core-debuginfo
%endif

%if "%_project" != "Kirana_SWA_OPEN:Build" && "%_project" != "Kirana_SWA_OPEN:Daily"
Requires:  swap-modules
%endif
Requires:  swap-probe
Requires:  swap-probe-elf
Requires:  sdbd
Requires:  libwebsockets

%description
SWAP manager is a part of data collection back-end for DA.
This binary will be installed in target.

%prep
%setup -q -n %{name}_%{version}

%build
pushd scripts
echo "__tizen_profile_name__="%{?tizen_profile_name} > dyn_vars
echo "__tizen_product_tv__="%{?TIZEN_PRODUCT_TV} >> dyn_vars
echo "__tizen_product_2_4_wearable__="%{sec_product_feature_profile_wearable} >> dyn_vars
popd
cd daemon

%if "%{?tizen_profile_name}" == "mobile"
SWAP_BUILD_CMD+=" CALL_MNGR=y"
%endif

%if "%{?tizen_profile_name}" == "tv"
SWAP_BUILD_CMD+=" PROFILE_TV=y"
%else
%if "%{sec_product_feature_profile_wearable}" == "1"
SWAP_BUILD_CMD+=" OLD_JSON=y"
%else
SWAP_BUILD_CMD+=" WSP_SUPPORT=y"
%endif
%endif

SWAP_BUILD_CMD+=" make"
eval ${SWAP_BUILD_CMD}

cd ../ui_viewer
$SWAP_BUILD_CONF make

%install
rm -rf ${RPM_BUILD_ROOT}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}
cd daemon
%make_install

cd ../ui_viewer
%make_install

%post
mkdir -p /opt/usr/etc
touch /opt/usr/etc/resourced_proc_exclude.ini

%files
/usr/share/license/%{name}
%manifest swap-manager.manifest
%defattr(-,root,root,-)
%{_prefix}/bin/da_manager
/usr/bin/swap_start.sh
/usr/bin/swap_stop.sh
/usr/bin/swap_init_loader.sh
/usr/bin/swap_init_preload.sh
/usr/bin/swap_init_uihv.sh
/usr/bin/swap_init_wsp.sh

%{_prefix}/lib/da_ui_viewer.so

%changelog
