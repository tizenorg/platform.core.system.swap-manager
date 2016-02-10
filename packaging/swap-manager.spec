Name:       swap-manager
Summary:    SWAP manager
Version:    3.0
Release:    1
Group:      System/Libraries
License:    Apache-2.0
Source:    %{name}_%{version}.tar.gz


# setup config
%define NSP_SUPPORT 0
%define WSP_SUPPORT 0
%define WSI_SUPPORT 0


ExcludeArch: aarch64 x86_64
BuildRequires: smack-devel
BuildRequires: libattr-devel
BuildRequires: glib2-devel
BuildRequires: aul-devel
BuildRequires: vconf-devel
BuildRequires: capi-system-info-devel
BuildRequires: capi-system-runtime-info-devel
BuildRequires: pkgconfig(json-c)
BuildRequires: pkgconfig(ecore)
BuildRequires: launchpad
BuildRequires: app-core-efl
BuildRequires: libwayland-egl
BuildRequires: app-core-efl-debuginfo
BuildRequires: swap-probe-devel
BuildRequires: swap-probe-elf


%define NSP_SUPPORT 1
# FIXME: add WSP_SUPPORT wrt webkit2-efl and webkit2-efl-debuginfo
# FIXME: add WSI_SUPPORT libwebsockets-devel

Requires: swap-modules
Requires: swap-probe
Requires: swap-probe-elf
Requires: sdbd

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

%if %{NSP_SUPPORT}
  export NSP_SUPPORT=y
%endif

%if %{WSP_SUPPORT}
  export WSP_SUPPORT=y
%endif

%if %{WSI_SUPPORT}
  export WSI_SUPPORT=y
%endif

make

%install
rm -rf ${RPM_BUILD_ROOT}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}
cd daemon
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
/usr/bin/swap_init_preload.sh
/usr/bin/swap_init_wsp.sh

%changelog

