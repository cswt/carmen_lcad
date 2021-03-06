include $(CARMEN_HOME)/src/Makefile.conf

MODULE_NAME = GRAPHSLAM_FAST_PACKAGE
MODULE_COMMENT = New Version of GraphSLAM for faster processing

LINK = g++ 

CXXFLAGS += -Wno-deprecated -Wno-deprecated-declarations -Wno-sign-compare

CFLAGS += -std=c11 -fopenmp
CXXFLAGS += -std=c++11 -fopenmp

# Just to prevent the process that generates the Makefile.depend from complaining we're not using C++11
IFLAGS += -std=c++11

PCL_INC = $(wildcard /usr/local/include/pcl-* /usr/include/pcl-*)
VTK_INC = $(wildcard /usr/local/include/vtk-5* /usr/include/vtk-5*)
IFLAGS += -I/usr/include/eigen3 -I $(PCL_INC) -I $(VTK_INC)

IFLAGS += -I$(CARMEN_HOME)/sharedlib/libtf/src -I/usr/local/include/bullet/  
IFLAGS += -I/usr/include/eigen3 
IFLAGS += -I/usr/include/suitesparse -I/usr/include/eigen3 -I/usr/local/include/EXTERNAL/csparse/ -I/usr/local/include/g2o/
IFLAGS += -I$(CARMEN_HOME)/sharedlib/prob_models -I$(CARMEN_HOME)/sharedlib/gl_plotter 
IFLAGS += `pkg-config --cflags opencv`

LFLAGS += -lglobal -lipc -llocalize_ackerman_interface -lfused_odometry_interface -lvelodyne_interface -lparam_interface -lGL -lglut -lGLU \
		-lrt -lboost_signals -std=c++11 # -ltf_util    
LFLAGS += -lrobot_ackerman_interface -lobstacle_avoider_interface \
		-lmap_io -lmap_util -llaser_interface -lfused_odometry_interface -lvelodyne_interface \
		-lGLEW -lGL -lGLU -lglut -lX11 -lmap_interface -lbase_ackerman_interface \
		-lstereo_velodyne -lstereo_velodyne_interface -lstereo_interface -llocalize_ackerman_core -llocalize_ackerman_interface \
		-llocalize_ackerman_motion -lmap_server_interface -lgps_xyz_interface -lvisual_odometry_interface -lxsens_mtig_interface \
		-lxsens_interface -lgps_nmea_interface
LFLAGS += -L /usr/local/lib -lcholmod -lg2o_core -lg2o_stuff -lg2o_types_slam2d -lg2o_cli
LFLAGS += -lvtkCommon -lvtkFiltering -lvtkRendering -lpcl_common -lpcl_surface -lpcl_io -lpcl_registration -lpcl_kdtree -lpcl_features -lpcl_search \
		-lpcl_octree -lpcl_sample_consensus -lpcl_filters -lpcl_visualization  -lboost_system -lboost_thread-mt
LFLAGS += -L$(CARMEN_HOME)/sharedlib/prob_models -L$(CARMEN_HOME)/sharedlib/gl_plotter -lprob_models -lrotation_geometry
LFLAGS += -lmapper_interface -lgrid_mapping -lmap_io -lmap_interface
LFLAGS += -L$(CARMEN_HOME)/sharedlib/libtf/src -ltf -lpthread -lBulletCollision -lBulletDynamics -lBulletSoftBody -lLinearMath -lboost_thread-mt -lrt -lboost_signals -lboost_system -lgomp -lvtkGraphics -lsegmap 
LFLAGS += `pkg-config --libs opencv` -lboost_system -lsegmap

IFLAGS += -I /usr/local/include/pcl-1.8 -I /usr/include/eigen3/ -I /usr/include/vtk-5.10/ `pkg-config --cflags opencv`
LFLAGS += -L /usr/local/lib -lpcl_common -lpcl_io -lpcl_visualization

SOURCES = generate_loop_closures.cpp graphslam.cpp joint_graphslam.cpp gicp_odometry_parallel.cpp graphslam_util.cpp \
	gicp_reconstruction.cpp gicp_odometry_view_result.cpp gicp_odometry_view_matching_result.cpp \
	gicp_odometry_manual_correction.cpp convert_loops_to_graphslam_format.cpp gicp_odometry_simple.cpp
TARGETS = graphslam joint_graphslam gicp_odometry gicp_reconstruction gicp_odometry_view_result generate_loop_closures \
	gicp_odometry_view_matching_result gicp_odometry_manual_correction convert_loops_to_graphslam_format gicp_odometry_simple

generate_loop_closures: generate_loop_closures.o graphslam_util.o ../libsegmap/libsegmap.a
graphslam: graphslam.o
joint_graphslam: joint_graphslam.o graphslam_util.o ../libsegmap/libsegmap.a
gicp_odometry: gicp_odometry_parallel.o graphslam_util.o ../libsegmap/libsegmap.a
gicp_reconstruction: gicp_reconstruction.o graphslam_util.o ../libsegmap/libsegmap.a
gicp_odometry_view_result: gicp_odometry_view_result.o graphslam_util.o ../libsegmap/libsegmap.a
gicp_odometry_view_matching_result: gicp_odometry_view_matching_result.o graphslam_util.o ../libsegmap/libsegmap.a
gicp_odometry_manual_correction: gicp_odometry_manual_correction.o graphslam_util.o ../libsegmap/libsegmap.a
convert_loops_to_graphslam_format: convert_loops_to_graphslam_format.o graphslam_util.o ../libsegmap/libsegmap.a
gicp_odometry_simple: gicp_odometry_simple.o graphslam_util.o ../libsegmap/libsegmap.a

include $(CARMEN_HOME)/src/Makefile.rules
