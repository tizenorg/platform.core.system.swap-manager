Name:       dynamic-analysis-manager
Summary:    dynamic analyzer manager
Version:    2.1.2
Release:    1
Group:      System/Libraries
License:	Apache License, Version 2
Source:    %{name}_%{version}.tar.gz
BuildRequires:  smack-devel
BuildRequires:  libattr-devel
BuildRequires:  glib2-devel
BuildRequires:  aul-devel
BuildRequires:  vconf-devel
BuildRequires:  capi-system-info-devel
BuildRequires:  capi-system-runtime-info-devel
BuildRequires:  capi-telephony-network-info-devel
BuildRequires:  capi-telephony-call-devel
Requires:	sdbd

%description
Dynamic analysis manager is for dynamic analyzer.
This binary will be installed in target.

%prep
%setup -q -n %{name}_%{version}

%build
cd daemon
make

%install
rm -rf ${RPM_BUILD_ROOT}
cd daemon
%make_install

%files
%manifest dynamic-analysis-manager.manifest
%defattr(-,root,root,-)
%{_prefix}/bin/da_manager
%{_prefix}/bin/da_command

%changelog

