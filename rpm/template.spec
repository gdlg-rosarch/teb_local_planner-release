Name:           ros-jade-teb-local-planner
Version:        0.1.11
Release:        0%{?dist}
Summary:        ROS teb_local_planner package

Group:          Development/Libraries
License:        BSD
URL:            http://wiki.ros.org/teb_local_planner
Source0:        %{name}-%{version}.tar.gz

Requires:       ros-jade-base-local-planner
Requires:       ros-jade-costmap-2d
Requires:       ros-jade-dynamic-reconfigure
Requires:       ros-jade-geometry-msgs
Requires:       ros-jade-interactive-markers
Requires:       ros-jade-libg2o
Requires:       ros-jade-message-runtime
Requires:       ros-jade-nav-core
Requires:       ros-jade-nav-msgs
Requires:       ros-jade-pluginlib
Requires:       ros-jade-roscpp
Requires:       ros-jade-std-msgs
Requires:       ros-jade-tf
Requires:       ros-jade-tf-conversions
Requires:       ros-jade-visualization-msgs
BuildRequires:  ros-jade-base-local-planner
BuildRequires:  ros-jade-catkin
BuildRequires:  ros-jade-cmake-modules
BuildRequires:  ros-jade-costmap-2d
BuildRequires:  ros-jade-dynamic-reconfigure
BuildRequires:  ros-jade-geometry-msgs
BuildRequires:  ros-jade-interactive-markers
BuildRequires:  ros-jade-libg2o
BuildRequires:  ros-jade-message-generation
BuildRequires:  ros-jade-nav-core
BuildRequires:  ros-jade-nav-msgs
BuildRequires:  ros-jade-pluginlib
BuildRequires:  ros-jade-roscpp
BuildRequires:  ros-jade-std-msgs
BuildRequires:  ros-jade-tf
BuildRequires:  ros-jade-tf-conversions
BuildRequires:  ros-jade-visualization-msgs

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
if [ -f "/opt/ros/jade/setup.sh" ]; then . "/opt/ros/jade/setup.sh"; fi
mkdir -p obj-%{_target_platform} && cd obj-%{_target_platform}
%cmake .. \
        -UINCLUDE_INSTALL_DIR \
        -ULIB_INSTALL_DIR \
        -USYSCONF_INSTALL_DIR \
        -USHARE_INSTALL_PREFIX \
        -ULIB_SUFFIX \
        -DCMAKE_INSTALL_PREFIX="/opt/ros/jade" \
        -DCMAKE_PREFIX_PATH="/opt/ros/jade" \
        -DSETUPTOOLS_DEB_LAYOUT=OFF \
        -DCATKIN_BUILD_BINARY_PACKAGE="1" \

make %{?_smp_mflags}

%install
# In case we're installing to a non-standard location, look for a setup.sh
# in the install tree that was dropped by catkin, and source it.  It will
# set things like CMAKE_PREFIX_PATH, PKG_CONFIG_PATH, and PYTHONPATH.
if [ -f "/opt/ros/jade/setup.sh" ]; then . "/opt/ros/jade/setup.sh"; fi
cd obj-%{_target_platform}
make %{?_smp_mflags} install DESTDIR=%{buildroot}

%files
/opt/ros/jade

%changelog
* Sat Dec 12 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.11-0
- Autogenerated by Bloom

* Thu Aug 13 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.10-0
- Autogenerated by Bloom

* Wed Jun 24 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.9-0
- Autogenerated by Bloom

* Mon Jun 08 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.8-0
- Autogenerated by Bloom

* Thu May 21 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.5-0
- Autogenerated by Bloom

* Wed May 20 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.4-0
- Autogenerated by Bloom

* Tue May 19 2015 Christoph Rösmann <christoph.roesmann@tu-dortmund.de> - 0.1.2-0
- Autogenerated by Bloom

