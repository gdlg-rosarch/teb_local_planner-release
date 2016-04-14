Name:           ros-indigo-teb-local-planner
Version:        0.3.1
Release:        1%{?dist}
Summary:        ROS teb_local_planner package

Group:          Development/Libraries
License:        BSD
URL:            http://wiki.ros.org/teb_local_planner
Source0:        %{name}-%{version}.tar.gz

Requires:       ros-indigo-base-local-planner
Requires:       ros-indigo-costmap-2d
Requires:       ros-indigo-costmap-converter
Requires:       ros-indigo-dynamic-reconfigure
Requires:       ros-indigo-geometry-msgs
Requires:       ros-indigo-interactive-markers
Requires:       ros-indigo-libg2o
Requires:       ros-indigo-message-runtime
Requires:       ros-indigo-nav-core
Requires:       ros-indigo-nav-msgs
Requires:       ros-indigo-pluginlib
Requires:       ros-indigo-roscpp
Requires:       ros-indigo-std-msgs
Requires:       ros-indigo-tf
Requires:       ros-indigo-tf-conversions
Requires:       ros-indigo-visualization-msgs
BuildRequires:  ros-indigo-base-local-planner
BuildRequires:  ros-indigo-catkin
BuildRequires:  ros-indigo-cmake-modules
BuildRequires:  ros-indigo-costmap-2d
BuildRequires:  ros-indigo-costmap-converter
BuildRequires:  ros-indigo-dynamic-reconfigure
BuildRequires:  ros-indigo-geometry-msgs
BuildRequires:  ros-indigo-interactive-markers
BuildRequires:  ros-indigo-libg2o
BuildRequires:  ros-indigo-message-generation
BuildRequires:  ros-indigo-nav-core
BuildRequires:  ros-indigo-nav-msgs
BuildRequires:  ros-indigo-pluginlib
BuildRequires:  ros-indigo-roscpp
BuildRequires:  ros-indigo-std-msgs
BuildRequires:  ros-indigo-tf
BuildRequires:  ros-indigo-tf-conversions
BuildRequires:  ros-indigo-visualization-msgs

%description
The teb_local_planner package implements a plugin to the base_local_planner of
the 2D navigation stack. The underlying method called Timed Elastic Band locally
optimizes the robot's trajectory with respect to trajectory execution time,
separation from obstacles and compliance with kinodynamic constraints at
runtime.

%prep
%setup -q

%build
# In case we're installing to a non-standard location, look for a setup.sh
# in the install tree that was dropped by catkin, and source it.  It will
# set things like CMAKE_PREFIX_PATH, PKG_CONFIG_PATH, and PYTHONPATH.
if [ -f "/opt/ros/indigo/setup.sh" ]; then . "/opt/ros/indigo/setup.sh"; fi
mkdir -p obj-%{_target_platform} && cd obj-%{_target_platform}
%cmake .. \
        -UINCLUDE_INSTALL_DIR \
        -ULIB_INSTALL_DIR \
        -USYSCONF_INSTALL_DIR \
        -USHARE_INSTALL_PREFIX \
        -ULIB_SUFFIX \
        -DCMAKE_INSTALL_LIBDIR="lib" \
        -DCMAKE_INSTALL_PREFIX="/opt/ros/indigo" \
        -DCMAKE_PREFIX_PATH="/opt/ros/indigo" \
        -DSETUPTOOLS_DEB_LAYOUT=OFF \
        -DCATKIN_BUILD_BINARY_PACKAGE="1" \

make %{?_smp_mflags}

%install
# In case we're installing to a non-standard location, look for a setup.sh
# in the install tree that was dropped by catkin, and source it.  It will
# set things like CMAKE_PREFIX_PATH, PKG_CONFIG_PATH, and PYTHONPATH.
if [ -f "/opt/ros/indigo/setup.sh" ]; then . "/opt/ros/indigo/setup.sh"; fi
cd obj-%{_target_platform}
make %{?_smp_mflags} install DESTDIR=%{buildroot}

%files
/opt/ros/indigo

%changelog
* Thu Apr 14 2016 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.3.1-1
- Autogenerated by Bloom

* Thu Apr 14 2016 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.3.1-0
- Autogenerated by Bloom

* Fri Apr 08 2016 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.3.0-0
- Autogenerated by Bloom

* Mon Feb 01 2016 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.2.3-0
- Autogenerated by Bloom

* Mon Jan 11 2016 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.2.2-0
- Autogenerated by Bloom

* Wed Dec 30 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.2.1-0
- Autogenerated by Bloom

* Wed Dec 23 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.2.0-0
- Autogenerated by Bloom

* Sat Dec 12 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.11-0
- Autogenerated by Bloom

* Thu Aug 13 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.10-0
- Autogenerated by Bloom

* Wed Jun 24 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.9-0
- Autogenerated by Bloom

* Mon Jun 08 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.8-0
- Autogenerated by Bloom

* Fri May 22 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.7-0
- Autogenerated by Bloom

* Fri May 22 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.6-0
- Autogenerated by Bloom

* Thu May 21 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.5-0
- Autogenerated by Bloom

* Wed May 20 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.3-0
- Autogenerated by Bloom

* Tue May 19 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.2-0
- Autogenerated by Bloom

* Tue May 19 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.1-0
- Autogenerated by Bloom

