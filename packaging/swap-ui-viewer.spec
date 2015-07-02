Name:       swap-ui-viewer
Summary:    SWAP UI viewer library
Version:    3.0
Release:    1
Group:      System/Libraries
License:    Apache-2.0
Source:    %{name}_%{version}.tar.gz

%description
SWAP UI viewer is a part of data collection back-end for DA.
This library will be installed in target.

%prep
%setup -q -n %{name}_%{version}

%build
pushd scripts
echo "__tizen_profile_name__="%{?tizen_profile_name} > dyn_vars
popd
cd ui_viewer

$SWAP_BUILD_CONF make

%install
rm -rf ${RPM_BUILD_ROOT}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}
cd ui_viewer
%make_install

%files
/usr/share/license/%{name}
%manifest swap-ui-viewer.manifest
%defattr(-,root,root,-)
%{_prefix}/lib/da_ui_viewer.so

%changelog
