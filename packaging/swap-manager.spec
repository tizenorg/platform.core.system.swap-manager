Name:       swap-manager
Summary:    SWAP manager
Version:    3.0
Release:    1
Group:      System/Libraries
License:    Apache-2.0
Source:    %{name}_%{version}.tar.gz

# TODO REMOVE IT!!!
#BuildRequires: coregl

BuildRequires:  smack-devel
BuildRequires:  libattr-devel
BuildRequires:  glib2-devel
BuildRequires:  aul-devel
BuildRequires:  vconf-devel
BuildRequires:  capi-system-info-devel
BuildRequires:  capi-system-runtime-info-devel
BuildRequires:  libwebsockets-devel
%if "%{sec_product_feature_profile_wearable}" == "1"
BuildRequires:  libjson-devel
%else
BuildRequires:  pkgconfig(json-c)
%endif
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(libsystemd-daemon)
%if "%{?tizen_profile_name}" == "mobile"
BuildRequires:  call-manager
BuildRequires:  libcall-manager-devel
%endif
#BuildRequires:  swap-probe-devel
#BuildRequires:  swap-probe-elf
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
%if "%{TIZEN_PRODUCT_TV}" != "1"
BuildRequires:  app-core-debuginfo
%endif
%if "%_project" != "Kirana_SWA_OPEN:Build" && "%_project" != "Kirana_SWA_OPEN:Daily"
Requires:  swap-modules
%endif
#Requires:  swap-probe
#Requires:  swap-probe-elf
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

%install
rm -rf ${RPM_BUILD_ROOT}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}

#systemd
mkdir -p %{buildroot}%{_libdir}/systemd/system

%ifarch %{ix86}
install -m 0644 swap.service %{buildroot}%{_libdir}/systemd/system/swap.service
install -m 0644 swap.init.service %{buildroot}%{_libdir}/systemd/system/swap.init.service
install -m 0644 swap.socket %{buildroot}%{_libdir}/systemd/system/swap.socket

mkdir -p %{buildroot}/%{_libdir}/systemd/system/sockets.target.wants/
mkdir -p %{buildroot}/%{_libdir}/systemd/system/emulator.target.wants
mkdir -p %{buildroot}/%{_libdir}/systemd/system/sockets.target.wants

ln -s %{_libdir}/systemd/system/swap.init.service %{buildroot}/%{_libdir}/systemd/system/sockets.target.wants/
ln -s %{_libdir}/systemd/system/swap.service %{buildroot}/%{_libdir}/systemd/system/emulator.target.wants/
ln -s %{_libdir}/systemd/system/swap.socket %{buildroot}/%{_libdir}/systemd/system/sockets.target.wants/
%else
install -m 0644 swap.service %{buildroot}%{_libdir}/systemd/system/swap.service
install -m 0644 swap.init.service %{buildroot}%{_libdir}/systemd/system/swap.init.service
install -m 0644 swap.socket %{buildroot}%{_libdir}/systemd/system/swap.socket

mkdir -p %{buildroot}/%{_libdir}/systemd/system/sockets.target.wants/
#mkdir -p %{buildroot}/%{_libdir}/systemd/system/multi-user.target.wants
mkdir -p %{buildroot}/%{_libdir}/systemd/system/sockets.target.wants

ln -s %{_libdir}/systemd/system/swap.init.service %{buildroot}/%{_libdir}/systemd/system/sockets.target.wants/
#ln -s %{_libdir}/systemd/system/swap.service %{buildroot}/%{_libdir}/systemd/system/multi-user.target.wants/
ln -s %{_libdir}/systemd/system/swap.socket %{buildroot}/%{_libdir}/systemd/system/sockets.target.wants/

%endif

cd daemon
%make_install


%files
#systemd
%{_libdir}/systemd/system/swap.init.service
%{_libdir}/systemd/system/swap.service
%{_libdir}/systemd/system/swap.socket

%ifarch %{ix86}
%{_libdir}/systemd/system/sockets.target.wants/swap.init.service
%{_libdir}/systemd/system/emulator.target.wants/swap.service
%{_libdir}/systemd/system/sockets.target.wants/swap.socket
%else
%{_libdir}/systemd/system/sockets.target.wants/swap.init.service
#%{_libdir}/systemd/system/multi-user.target.wants/swap.service
%{_libdir}/systemd/system/sockets.target.wants/swap.socket
%endif


/usr/share/license/%{name}
%manifest swap-manager.manifest
%defattr(-,root,root,-)
/opt/swap/sdk/service_init.sh
/opt/swap/sdk/service_preinit.sh

%defattr(-,developer,developer,-)
%{_prefix}/bin/da_manager
/opt/swap/sdk/start.sh
/opt/swap/sdk/stop.sh
/opt/swap/sdk/init_preload.sh

%changelog

