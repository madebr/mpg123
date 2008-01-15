Summary:	The fast console mpeg audio decoder/player.
Name:		mpg123
Version:	1.0.1
Release:	3
URL:		http://www.mpg123.org/
License:	GPL
Group:		Applications/Multimedia
Packager:	Michael Ryzhykh <mclroy@gmail.com>
Source:		http://www.mpg123.org/download/mpg123-%{version}.tar.bz2
Patch0:		mpg123-1.0.1.patch0
BuildRoot:	%_tmppath/%name-%version
Prefix: 	/usr

%description
This is a console based decoder/player for mono/stereo mpeg audio files,
probably more familiar as MP3 or MP2 files. It's focus is speed.
It can play MPEG1.0/2.0/2.5 layer I, II, II (1, 2, 3;-) files
(VBR files are fine, too) and produce output on a number of different ways:
raw data to stdout and different sound systems depending on your platform.

%package output
Summary:	The set of audio output modules for mpg123
Group:		Applications/Multimedia

%description output
The set of audio output modules for mpg123.
The full list is "alsa oss coreaudio sun win32 esd jack portaudio pulse sdl nas dummy".

%prep
%setup -q -n %name-%version
%patch0 -p1

%build
%configure --with-cpu=x86 --with-default-audio=alsa --enable-ltdl-install=yes
make

%install
%{__rm} -rf %{buildroot}
%makeinstall

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(755,root,root)
%{_bindir}/*
%defattr(644,root,root)
%doc %{_mandir}/*/*
%{_libdir}/pkgconfig/libmpg123.*
%{_libdir}/libmpg123.*a*
%{_libdir}/libmpg123.so*
%{_libdir}/libltdl.*a*
%{_libdir}/libltdl.so*
%{_includedir}/*.h

%files output
%defattr(644,root,root)
%{_libdir}/mpg123/output_*.*a*
%{_libdir}/mpg123/output_*.so*

%changelog
* Tue Jan  1 2008 Michael Ryzhykh <mclroy@gmail.com>
- Initial Version.

