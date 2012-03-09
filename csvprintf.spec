#
# csvprintf - Simple CSV file parser for the UNIX command line
#
# Copyright 2010 Archie L. Cobbs <archie@dellroad.org>
# 
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
# 
# $Id$
#

Name:           csvprintf
Version:        1.0.2
Release:        1
License:        Apache License, Version 2.0
Summary:        Simple CSV file parser for the UNIX command line
Group:          Productivity/Text/Utilities
Source:         %{name}-%{version}.tar.gz
URL:            http://%{name}.googlecode.com/
BuildRoot:      %{_tmppath}/%{name}-%{version}-root
BuildRequires:  make
BuildRequires:  gcc

%description
%{name} is a simple UNIX command line utility for parsing CSV files.

%{name} works just like the printf(1) command line utility. You
supply a printf(1) format string on the command line and each record
in the CSV file is formatted accordingly. Each format specifier in
the format string contains a column accessor to specify which CSV
column to use, so for example '%%3$d' would format the third column
as a decimal value.

%{name} can also convert CSV files into XML documents.

%prep
%setup -q

%build
%{configure}
make

%install
rm -rf ${RPM_BUILD_ROOT}
%{makeinstall}

%files
%attr(0755,root,root) %{_bindir}/%{name}
%attr(0644,root,root) %{_mandir}/man1/%{name}.1.gz
%defattr(0644,root,root,0755)
%doc %{_datadir}/doc/packages/%{name}

%changelog
