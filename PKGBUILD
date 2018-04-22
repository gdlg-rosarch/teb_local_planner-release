# Script generated with Bloom
pkgdesc="ROS - The teb_local_planner package implements a plugin to the base_local_planner of the 2D navigation stack. The underlying method called Timed Elastic Band locally optimizes the robot's trajectory with respect to trajectory execution time, separation from obstacles and compliance with kinodynamic constraints at runtime."
url='http://wiki.ros.org/teb_local_planner'

pkgname='ros-kinetic-teb-local-planner'
pkgver='0.6.7_1'
pkgrel=1
arch=('any')
license=('BSD'
)

makedepends=('ros-kinetic-base-local-planner'
'ros-kinetic-catkin'
'ros-kinetic-cmake-modules'
'ros-kinetic-costmap-2d'
'ros-kinetic-costmap-converter'
'ros-kinetic-dynamic-reconfigure'
'ros-kinetic-geometry-msgs'
'ros-kinetic-interactive-markers'
'ros-kinetic-libg2o'
'ros-kinetic-message-generation'
'ros-kinetic-nav-core'
'ros-kinetic-nav-msgs'
'ros-kinetic-pluginlib'
'ros-kinetic-roscpp'
'ros-kinetic-std-msgs'
'ros-kinetic-tf'
'ros-kinetic-tf-conversions'
'ros-kinetic-visualization-msgs'
)

depends=('ros-kinetic-base-local-planner'
'ros-kinetic-costmap-2d'
'ros-kinetic-costmap-converter'
'ros-kinetic-dynamic-reconfigure'
'ros-kinetic-geometry-msgs'
'ros-kinetic-interactive-markers'
'ros-kinetic-libg2o'
'ros-kinetic-message-runtime'
'ros-kinetic-nav-core'
'ros-kinetic-nav-msgs'
'ros-kinetic-pluginlib'
'ros-kinetic-roscpp'
'ros-kinetic-std-msgs'
'ros-kinetic-tf'
'ros-kinetic-tf-conversions'
'ros-kinetic-visualization-msgs'
)

conflicts=()
replaces=()

_dir=teb_local_planner
source=()
md5sums=()

prepare() {
    cp -R $startdir/teb_local_planner $srcdir/teb_local_planner
}

build() {
  # Use ROS environment variables
  source /usr/share/ros-build-tools/clear-ros-env.sh
  [ -f /opt/ros/kinetic/setup.bash ] && source /opt/ros/kinetic/setup.bash

  # Create build directory
  [ -d ${srcdir}/build ] || mkdir ${srcdir}/build
  cd ${srcdir}/build

  # Fix Python2/Python3 conflicts
  /usr/share/ros-build-tools/fix-python-scripts.sh -v 2 ${srcdir}/${_dir}

  # Build project
  cmake ${srcdir}/${_dir} \
        -DCMAKE_BUILD_TYPE=Release \
        -DCATKIN_BUILD_BINARY_PACKAGE=ON \
        -DCMAKE_INSTALL_PREFIX=/opt/ros/kinetic \
        -DPYTHON_EXECUTABLE=/usr/bin/python2 \
        -DPYTHON_INCLUDE_DIR=/usr/include/python2.7 \
        -DPYTHON_LIBRARY=/usr/lib/libpython2.7.so \
        -DPYTHON_BASENAME=-python2.7 \
        -DSETUPTOOLS_DEB_LAYOUT=OFF
  make
}

package() {
  cd "${srcdir}/build"
  make DESTDIR="${pkgdir}/" install
}

