cmake_minimum_required(VERSION 3.1.0)
project(random_points)
include_directories(include/ /usr/local/include)
add_executable(random_points
	src/main.c 
	src/linear.c
	src/eglut/eglut.c
	src/eglut/eglut_wayland.c
	src/eglut/wayland-xdg-shell-protocol.c
	#src/eglut/eglut_x11.c
	)
target_link_libraries(random_points GLESv2 libEGL.so.0 wayland-client m)
#target_link_libraries(random_points GLESv2 X11 m)
