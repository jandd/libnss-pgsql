%define ver	1.4.0
%define rel 1
%define prefix /usr
%define sysconfdir /etc

Summary: PostgreSQL NSS module
Name: libnss-pgsql
Version: %ver
Release: %rel
Copyright: GPL 2.0
Group: System Environment/Libraries
Source: libnss-pgsql-%{PACKAGE_VERSION}.tar.gz
URL: http://www.pgfoundry.org/projects/sysauth/
BuildRoot: /tmp/libnss-pgsql-%{PACKAGE_VERSION}-build
Packager: Bret Mogilefsky <mogul-sysauth-pgsql@gelatinous.com>

%description
libnss-pgsql is a name service switch module that allows the 
replacement of flatfile passwd, group and shadow lookups with a 
PostgreSQL backend.

%prep
%setup
cd $RPM_BUILD_DIR/%{name}-%{version}
mkdir -p %{bldroot}%{prefix}
cd $RPM_BUILD_DIR/%{name}-%{version}
./configure --prefix=%{prefix} --sysconfdir=%{sysconfdir}


%build
make

%install
rm -fr $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install
# mkdir ${RPM_BUILD_ROOT}%{sysconfdir}
# cp conf/nss-pgsql.conf ${RPM_BUILD_ROOT}%{sysconfdir}
# cp conf/nss-pgsql-root.conf ${RPM_BUILD_ROOT}%{sysconfdir}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-, root, root)
%doc AUTHORS COPYING ChangeLog INSTALL NEWS README doc/nss-pgsql.txt doc/nss-pgsql.html doc/nss-pgsql.sgml
%{prefix}/lib/*.so*
%{prefix}/lib/*.la
# %attr(0644,root,root) %config(noreplace) %{sysconfdir}/nss-pgsql.conf
# %attr(0600,root,root) %config(noreplace) %{sysconfdir}/nss-pgsql-root.conf
