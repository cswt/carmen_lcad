#include "neural_object_detector2.hpp"
#include <tf.h>

#define SHOW_DETECTIONS

using namespace std;

int camera;
int camera_side;

carmen_camera_parameters camera_parameters;
carmen_pose_3D_t velodyne_pose;
carmen_pose_3D_t camera_pose;
carmen_pose_3D_t board_pose;

const unsigned int maxPositions = 50;
carmen_velodyne_partial_scan_message *velodyne_message_arrange;
vector<carmen_velodyne_partial_scan_message> velodyne_vector;


Detector *darknet;
vector<string> obj_names;


carmen_moving_objects_point_clouds_message moving_objects_point_clouds_message;
carmen_point_t globalpos;

carmen_ackerman_traj_point_t rddf_msg;
bool goal_ready, use_rddf;

carmen_behavior_selector_road_profile_message goal_list_message;

carmen_rddf_annotation_message last_rddf_annotation_message;
carmen_behavior_selector_road_profile_message last_rddf_poses;

bool last_rddf_annotation_message_valid = false;





/*tf::Point
move_point_to_camera_reference (tf::Point point, carmen_pose_3D_t camera_pose)
{
	//cout<<camera_pose.orientation.yaw<<" "<<camera_pose.orientation.pitch<<" "<<camera_pose.orientation.roll<<endl;
	tf::Transform pose_camera_in_board(
			tf::Quaternion(camera_pose.orientation.yaw, camera_pose.orientation.pitch, camera_pose.orientation.roll),
			tf::Vector3(camera_pose.position.x, camera_pose.position.y, camera_pose.position.z));

	tf::Transform board_frame_to_camera_frame = pose_camera_in_board.inverse();
	return board_frame_to_camera_frame * point;

}*/


carmen_ackerman_traj_point_t
convert_rddf_pose_to_car_reference (carmen_ackerman_traj_point_t pose)
{
	carmen_ackerman_traj_point_t p;

	p.x = pose.x + globalpos.x;
	p.y = pose.y + globalpos.y;

	printf("Pose in car reference: %lf X %lf\n", p.x, p.y);

	return (p);
}

void               //rddf_x    rddf_y     car_x                car_y
carmen_translate_2d(double *x, double *y, double new_origin_x, double new_origin_y)
{
	*x = abs(*x);
	*y = abs(*y);
	new_origin_x = abs(new_origin_x);
	new_origin_y = abs(new_origin_y);

	if(*x > new_origin_x)
	{
		*x -= new_origin_x;
	}
	else
	{
		*x = new_origin_x - *x;
	}

	if(*y > new_origin_y)
	{
		*y -= new_origin_y;
	}
	else
	{
		*y = new_origin_y - *y;
	}
}


tf::Point
move_to_camera_reference2(tf::Point p3d_velodyne_reference, carmen_pose_3D_t velodyne_pose, carmen_pose_3D_t camera_pose)
{
    tf::Transform pose_velodyne_in_board(
            tf::Quaternion(velodyne_pose.orientation.yaw, velodyne_pose.orientation.pitch, velodyne_pose.orientation.roll),
            tf::Vector3(velodyne_pose.position.x, velodyne_pose.position.y, velodyne_pose.position.z));

    tf::Transform pose_camera_in_board(
            tf::Quaternion(camera_pose.orientation.yaw, camera_pose.orientation.pitch, camera_pose.orientation.roll),
            tf::Vector3(camera_pose.position.x, camera_pose.position.y, camera_pose.position.z));


	tf::Transform velodyne_frame_to_board_frame = pose_velodyne_in_board;
	tf::Transform board_frame_to_camera_frame = pose_camera_in_board.inverse();

	return board_frame_to_camera_frame * velodyne_frame_to_board_frame * p3d_velodyne_reference;
}


carmen_position_t
convert_rddf_pose_to_point_in_image(carmen_ackerman_traj_point_t pose,
									carmen_camera_parameters camera_parameters,
									carmen_pose_3D_t camera_pose,
									int image_width, int image_height)
{
	carmen_position_t point;
	carmen_ackerman_traj_point_t pose_in_car_reference;

	tf::Point rddf_pose (pose.x, pose.y, 0.0);

	carmen_pose_3D_t world_ref;
	world_ref.position.x = 0.0;
	world_ref.position.y = 0.0;
	world_ref.position.z = 0.0;
	world_ref.orientation.pitch = 0.0;
	world_ref.orientation.roll = 0.0;
	world_ref.orientation.yaw = 0.0;


	carmen_pose_3D_t car_pos;
	car_pos.position.x = globalpos.x;
	car_pos.position.y = globalpos.y;
	car_pos.position.z = 0.0;
	car_pos.orientation.pitch = 0.0;
	car_pos.orientation.roll = 0.0;
	car_pos.orientation.yaw = carmen_normalize_theta(globalpos.theta);


	tf::Point point_tf;
	tf::Point point_in_car;
	tf::Point point_in_camera;
	tf::Point point_in_board;

	// fx and fy are the focal lengths
	double fx_meters = camera_parameters.fx_factor * image_width * camera_parameters.pixel_size;
	double fy_meters = camera_parameters.fy_factor * image_height * camera_parameters.pixel_size;

	//printf("Focal Lenght: %lf X %lf\n", fx_meters, fy_meters);

	//cu, cv represent the camera principal point
	double cu = camera_parameters.cu_factor * (double) image_width;
	double cv = camera_parameters.cv_factor * (double) image_height;

	//printf("Principal Point: %lf X %lf\n", cu, cv);


	printf("Global Pos: %lf X %lf\n", globalpos.x, globalpos.y);
	printf("RDDF Pose: %lf X %lf\n", pose.x, pose.y);

	//pose_in_car_reference = convert_rddf_pose_to_car_reference (pose);
	//carmen_translate_2d(&pose.x, &pose.y, globalpos.x, globalpos.y);
	//move_to_camera_reference(tf::Point p3d_velodyne_reference, carmen_pose_3D_t velodyne_pose, carmen_pose_3D_t camera_pose)
	point_in_car = move_to_camera_reference2(rddf_pose, world_ref, car_pos);

	//carmen_rotate_2d(&pose.x, &pose.y, globalpos.theta);
	printf("Pose in car reference: %lf X %lf\n", point_in_car.x(), point_in_car.y());

	point_tf[0] = pose.x;
	point_tf[1] = pose.y;
	point_tf[2] = 0.0;//(pose.x, pose.y, 0.0);

	//camera_pose.position.x += 0.572;
	//camera_pose.position.y += 0.0;
	//camera_pose.position.z += 1.394;

	//point_in_board = move_to_camera_reference2(point_in_car, world_ref, board_pose);
	//printf("Pose in board reference: %lf X %lf\n", point_in_board.x(), point_in_car.y());

	point_in_camera = move_to_camera_reference2(point_in_car, world_ref, camera_pose);
	pose.x = point_in_camera[0];
	pose.y = point_in_camera[1];
	//carmen_translate_2d(&pose.x, &pose.y, camera_pose.position.x, camera_pose.position.y);


	printf("Pose in camera: %lf X %lf\n", pose.x, pose.y);
	//point_in_camera[0] = pose.x;
	//point_in_camera[1] = pose.y;
	//point_in_camera[2] = 0.0;//(pose.x, pose.y, 0.0);

	double px = (fx_meters * (pose.y / pose.x) / camera_parameters.pixel_size + cu);
	double py = (fy_meters * (camera_pose.position.z / pose.x) / camera_parameters.pixel_size + cv);
	printf("Pose in image: %lf X %lf\n\n", px, py);
	point.x = image_width - px;
	point.y = py;

	/*printf("Image Size: %d X %d\n", image_width, image_height);
	printf("Pose in global: %lf X %lf\n", pose.x, pose.y);
	printf("Pose in camera: %lf X %lf\n", point_in_camera.x(), point_in_camera.y());
	printf("Pose in image: %lf X %lf\n", point.x, point.y);
	getchar();*/
	return (point);

}


// This function find the closest velodyne message with the camera message
carmen_velodyne_partial_scan_message
find_velodyne_most_sync_with_cam(double bumblebee_timestamp)  // TODO is this necessary?
{
    carmen_velodyne_partial_scan_message velodyne;
    double minTimestampDiff = DBL_MAX;
    int minTimestampIndex = -1;

    for (unsigned int i = 0; i < velodyne_vector.size(); i++)
    {
        if (fabs(velodyne_vector[i].timestamp - bumblebee_timestamp) < minTimestampDiff)
        {
            minTimestampIndex = i;
            minTimestampDiff = fabs(velodyne_vector[i].timestamp - bumblebee_timestamp);
        }
    }

    velodyne = velodyne_vector[minTimestampIndex];
    return (velodyne);
}


void
build_moving_objects_message(vector<carmen_tracked_cluster_t> clusters)
{

    moving_objects_point_clouds_message.num_point_clouds = clusters.size();
    moving_objects_point_clouds_message.point_clouds = (t_point_cloud_struct *) (malloc(
            moving_objects_point_clouds_message.num_point_clouds * sizeof(t_point_cloud_struct)));


    for (int i = 0; i < moving_objects_point_clouds_message.num_point_clouds; i++) {
        carmen_vector_3D_t box_centroid = compute_centroid(clusters[i].points);
        carmen_vector_3D_t offset;

        // TODO ler do carmen ini
        offset.x = 0.572;
        offset.y = 0.0;
        offset.z = 2.154;

        box_centroid = translate_point(box_centroid, offset);
        box_centroid = rotate_point(box_centroid, globalpos.theta);

        offset.x = globalpos.x;
        offset.y = globalpos.y;
        offset.z = 0.0;

        box_centroid = translate_point(box_centroid, offset);

        moving_objects_point_clouds_message.point_clouds[i].r = 1.0;
        moving_objects_point_clouds_message.point_clouds[i].g = 1.0;
        moving_objects_point_clouds_message.point_clouds[i].b = 0.0;

        moving_objects_point_clouds_message.point_clouds[i].linear_velocity = 0;//clusters[i].linear_velocity;
        moving_objects_point_clouds_message.point_clouds[i].orientation = globalpos.theta;//clusters[i].orientation;

        moving_objects_point_clouds_message.point_clouds[i].object_pose.x = box_centroid.x;
        moving_objects_point_clouds_message.point_clouds[i].object_pose.y = box_centroid.y;
        moving_objects_point_clouds_message.point_clouds[i].object_pose.z = box_centroid.z;

        moving_objects_point_clouds_message.point_clouds[i].height = 1.8;
        moving_objects_point_clouds_message.point_clouds[i].length = 4.5;
        moving_objects_point_clouds_message.point_clouds[i].width = 1.6;

        switch (clusters[i].cluster_type) {

            case carmen_moving_object_type::pedestrian:
                moving_objects_point_clouds_message.point_clouds[i].geometric_model = 0;
                moving_objects_point_clouds_message.point_clouds[i].model_features.geometry.height = 1.8;
                moving_objects_point_clouds_message.point_clouds[i].model_features.geometry.length = 1.0;
                moving_objects_point_clouds_message.point_clouds[i].model_features.geometry.width = 1.0;
                moving_objects_point_clouds_message.point_clouds[i].model_features.red = 1.0;
                moving_objects_point_clouds_message.point_clouds[i].model_features.green = 1.0;
                moving_objects_point_clouds_message.point_clouds[i].model_features.blue = 0.8;
                moving_objects_point_clouds_message.point_clouds[i].model_features.model_name = (char *) "pedestrian";

                break;

            case carmen_moving_object_type::car:
                moving_objects_point_clouds_message.point_clouds[i].geometric_model = 0;
                moving_objects_point_clouds_message.point_clouds[i].model_features.geometry.height = 1.8;
                moving_objects_point_clouds_message.point_clouds[i].model_features.geometry.length = 4.5;
                moving_objects_point_clouds_message.point_clouds[i].model_features.geometry.width = 1.6;
                moving_objects_point_clouds_message.point_clouds[i].model_features.red = 1.0;
                moving_objects_point_clouds_message.point_clouds[i].model_features.green = 0.0;
                moving_objects_point_clouds_message.point_clouds[i].model_features.blue = 0.8;
                moving_objects_point_clouds_message.point_clouds[i].model_features.model_name = (char *) "car";

                break;
            default:
                moving_objects_point_clouds_message.point_clouds[i].geometric_model = 0;
                moving_objects_point_clouds_message.point_clouds[i].model_features.geometry.height = 1.8;
                moving_objects_point_clouds_message.point_clouds[i].model_features.geometry.length = 4.5;
                moving_objects_point_clouds_message.point_clouds[i].model_features.geometry.width = 1.6;
                moving_objects_point_clouds_message.point_clouds[i].model_features.red = 1.0;
                moving_objects_point_clouds_message.point_clouds[i].model_features.green = 1.0;
                moving_objects_point_clouds_message.point_clouds[i].model_features.blue = 0.0;
                moving_objects_point_clouds_message.point_clouds[i].model_features.model_name = (char *) "other";


                break;
        }

        moving_objects_point_clouds_message.point_clouds[i].num_associated = clusters[i].track_id;

        // fill the points
        moving_objects_point_clouds_message.point_clouds[i].point_size = clusters[i].points.size();
        moving_objects_point_clouds_message.point_clouds[i].points = (carmen_vector_3D_t *)
                malloc(moving_objects_point_clouds_message.point_clouds[i].point_size * sizeof(carmen_vector_3D_t));
        for (int j = 0; j < moving_objects_point_clouds_message.point_clouds[i].point_size; j++) {
            //TODO modificar isso
            carmen_vector_3D_t p;

            p.x = clusters[i].points[j].x;
            p.y = clusters[i].points[j].y;
            p.z = clusters[i].points[j].z;

            offset.x = 0.572;
            offset.y = 0.0;
            offset.z = 2.154;

            p = translate_point(p, offset);

            p = rotate_point(p, globalpos.theta);

            offset.x = globalpos.x;
            offset.y = globalpos.y;
            offset.z = 0.0;

            p = translate_point(p, offset);

            moving_objects_point_clouds_message.point_clouds[i].points[j] = p;
        }

    }

}


vector<string>
objects_names_from_file(string const class_names_file)
{
    ifstream file(class_names_file);
    vector<string> file_lines;

    if (!file.is_open())
    	return file_lines;

    for (string line; getline(file, line);)
    	file_lines.push_back(line);

    cout << "object names loaded \n";

    return file_lines;
}


///////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                           //
// Publishers                                                                                //
//                                                                                           //
///////////////////////////////////////////////////////////////////////////////////////////////


void
publish_moving_objects_message(double timestamp)
{
    moving_objects_point_clouds_message.timestamp = timestamp;
    moving_objects_point_clouds_message.host = carmen_get_host();

    carmen_moving_objects_point_clouds_publish_message(&moving_objects_point_clouds_message);

    for (int i = 0; i < moving_objects_point_clouds_message.num_point_clouds; i++) {
        free(moving_objects_point_clouds_message.point_clouds[i].points);
    }
    free(moving_objects_point_clouds_message.point_clouds);
}


///////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                           //
// Handlers                                                                                  //
//                                                                                           //
///////////////////////////////////////////////////////////////////////////////////////////////


void
rddf_handler(carmen_behavior_selector_road_profile_message *message)
{
	last_rddf_poses = *message;
	/*printf("RDDF NUM POSES: %d \n", message->number_of_poses);

	for (int i = 0; i < message->number_of_poses; i++)
	{
		printf("RDDF %d: x  = %lf, y = %lf , theta = %lf\n", i, last_rddf_poses.poses[i].x, last_rddf_poses.poses[i].y, last_rddf_poses.poses[i].theta);

	}*/
}


static void
rddf_annotation_message_handler(carmen_rddf_annotation_message *message)
{
	last_rddf_annotation_message = *message;
	last_rddf_annotation_message_valid = true;

	/*printf("RDDF NUM OF ANNOTATIONS: %d \n", last_rddf_annotation_message.num_annotations);

	for (int i = 0; i < message->num_annotations; i++)
	{
		printf("ANNOTATION %d: x  = %d\n", i, last_rddf_annotation_message.annotations->annotation_type);

	}*/
}


void
show_detections(cv::Mat rgb_image, vector<vector<carmen_velodyne_points_in_cam_with_obstacle_t>> laser_points_in_camera_box_list,
		vector<bbox_t> predictions, vector<bounding_box> bouding_boxes_list, double hood_removal_percentage, double fps, vector<carmen_position_t> rddf_points)
{
    char confianca[25];
    char frame_rate[25];

    sprintf(frame_rate, "FPS = %.2f", fps);

    cv::putText(rgb_image, frame_rate, cv::Point(10, 25), cv::FONT_HERSHEY_PLAIN, 2, cvScalar(0, 255, 0), 2);

    for (unsigned int i = 0; i < bouding_boxes_list.size(); i++)
    {

		for (unsigned int j = 0; j < laser_points_in_camera_box_list[i].size(); j++)
			cv::circle(rgb_image, cv::Point(laser_points_in_camera_box_list[i][j].velodyne_points_in_cam.ipx,
					laser_points_in_camera_box_list[i][j].velodyne_points_in_cam.ipy), 1, cv::Scalar(0, 0, 255), 1);

        cv::Scalar object_color;

        sprintf(confianca, "%d  %.3f", predictions.at(i).obj_id, predictions.at(i).prob);

        int obj_id = predictions.at(i).obj_id;

        string obj_name;
        if (obj_names.size() > obj_id)
            obj_name = obj_names[obj_id];

        if (obj_name.compare("car") == 0)
            object_color = cv::Scalar(0, 0, 255);
        else
            object_color = cv::Scalar(255, 0, 255);

        cv::rectangle(rgb_image,
                      cv::Point(bouding_boxes_list[i].pt1.x, bouding_boxes_list[i].pt1.y),
                      cv::Point(bouding_boxes_list[i].pt2.x, bouding_boxes_list[i].pt2.y),
                      object_color, 1);

        cv::putText(rgb_image, obj_name,
                    cv::Point(bouding_boxes_list[i].pt2.x + 1, bouding_boxes_list[i].pt1.y - 3),
                    cv::FONT_HERSHEY_PLAIN, 1, cvScalar(0, 0, 255), 1);

        cv::putText(rgb_image, confianca,
                    cv::Point(bouding_boxes_list[i].pt1.x + 1, bouding_boxes_list[i].pt1.y - 3),
                    cv::FONT_HERSHEY_PLAIN, 1, cvScalar(255, 255, 0), 1);

    }

    int thickness = -1;
    int lineType = 8;
    for (unsigned int i = 0; i < rddf_points.size(); i++)
    {
    	cv::circle(rgb_image, cv::Point(rddf_points[i].x, rddf_points[i].y), 3.5, cv::Scalar(0, 255, 255), thickness, lineType);
    }



    //cv::Mat resized_image(cv::Size(640, 480 - 480 * hood_removal_percentage), CV_8UC3);
    //cv::resize(rgb_image, resized_image, resized_image.size());

    cv::resize(rgb_image, rgb_image, Size(600, 300));
    cv::imshow("Neural Object Detector", rgb_image);
    cv::waitKey(1);

    //resized_image.release();
}


#define crop_x 0.0
#define crop_y 1.0
void
image_handler(carmen_bumblebee_basic_stereoimage_message *image_msg)
{
	double hood_removal_percentage = 0.2;
	carmen_velodyne_partial_scan_message velodyne_sync_with_cam;
	vector<carmen_tracked_cluster_t> clusters;
	vector<bounding_box> bouding_boxes_list;

    cv::Mat src_image = cv::Mat(cv::Size(image_msg->width, image_msg->height - image_msg->height * hood_removal_percentage), CV_8UC3);
    cv::Mat rgb_image = cv::Mat(cv::Size(image_msg->width, image_msg->height - image_msg->height * hood_removal_percentage), CV_8UC3);

    static double start_time = 0.0;
	double fps;

    if (camera_side == 0)
        memcpy(src_image.data, image_msg->raw_left, image_msg->image_size * sizeof(char) - image_msg->image_size * hood_removal_percentage * sizeof(char));
    else
        memcpy(src_image.data, image_msg->raw_right, image_msg->image_size * sizeof(char) - image_msg->image_size * hood_removal_percentage * sizeof(char));

    if (velodyne_vector.size() > 0)
        velodyne_sync_with_cam = find_velodyne_most_sync_with_cam(image_msg->timestamp); // TODO não faz sentido! Tem que sempre pegar a ultima msg do velodyne
    else
        return;

    cv::Mat src_image_copy = src_image.clone();

    cv::Mat pRoi = src_image_copy(cv::Rect(src_image_copy.cols * crop_x / 2.0, 0,
    		src_image_copy.cols - src_image_copy.cols * crop_x, src_image_copy.rows));
    src_image = pRoi;

    cv::cvtColor(src_image, rgb_image, cv::COLOR_RGB2BGR);

    int thickness = -1;
    int lineType = 8;
    carmen_position_t p;
    vector<carmen_position_t> rddf_points ;
    cv::Mat out;
    //cv::flip(rgb_image, out, 1);
    out = rgb_image;
    for(int i = 4; i < last_rddf_poses.number_of_poses; i++){
    	cout<<i<<endl;
    	p = convert_rddf_pose_to_point_in_image (last_rddf_poses.poses[i],camera_parameters, camera_pose,image_msg->width, image_msg->height);
    	rddf_points.push_back(p);
    	cv::circle(out, cv::Point(p.x, p.y), 3.5, cv::Scalar(0, 255, 255), thickness, lineType);
    }

    cv::imshow("test", out);
    cv::waitKey(10);

    vector<bbox_t> predictions;// = darknet->detect(src_image, 0.2);  // Arguments (img, threshold)

//    predictions = darknet->tracking(predictions); // Coment this line if object tracking is not necessary
    //INSERIR FUNÇÃO PARA FOVEADO: receber alguns pontos do rddf (função que converte da posição do mundo para a posição na imagem) recortar a área em volta do ponto
    //na imagem, passar essa imagem para a rede, receber a detecção e reprojetar na imagem original
    for (const auto &box : predictions) // Covert Darknet bounding box to neural_object_deddtector bounding box
    {
        bounding_box bbox;

        bbox.pt1.x = box.x;
        bbox.pt1.y = box.y;
        bbox.pt2.x = box.x + box.w;
        bbox.pt2.y = box.y + box.h;

        bouding_boxes_list.push_back(bbox);
    }

    // Removes the ground, Removes points outside cameras field of view and Returns the points that are obstacles and are inside bboxes
    vector<vector<carmen_velodyne_points_in_cam_with_obstacle_t>> laser_points_in_camera_box_list = velodyne_points_in_boxes(bouding_boxes_list,
    		&velodyne_sync_with_cam, camera_parameters, velodyne_pose, camera_pose, image_msg->width, image_msg->height);

    //std::vector<carmen_velodyne_points_in_cam_t> test;
    //test = carmen_velodyne_camera_calibration_lasers_points_in_camera(velodyne_message_arrange, camera_parameters, velodyne_pose, camera_pose, image_msg->width, image_msg->height);
    //printf("Image has %d X %d and has %lu points", image_msg->width, image_msg->height, test.size());getchar();
    //for(int i=0; i<test.size();i++){
    //	printf("%d X %d\n", test[i].ipx, test[i].ipy);
    //	if(i%30==0)
    //		getchar();
    //}



    // Removes the ground, Removes points outside cameras field of view and Returns the points that reach obstacles
    //vector<velodyne_camera_points> points = velodyne_camera_calibration_remove_points_out_of_FOV_and_ground(
    //		&velodyne_sync_with_cam, camera_parameters, velodyne_pose, camera_pose, image_msg->width, image_msg->height);

    // ONLY Convert from sferical to cartesian cordinates
    vector< vector<carmen_vector_3D_t>> cluster_list = get_cluster_list(laser_points_in_camera_box_list);

    // Cluster points and get biggest
    filter_points_in_clusters(&cluster_list);

    for (int i = 0; i < cluster_list.size(); i++)
    {
        carmen_moving_object_type tp = find_cluster_type_by_obj_id(obj_names, predictions.at(i).obj_id);

        int cluster_id = predictions.at(i).track_id;

        carmen_tracked_cluster_t clust;

        clust.points = cluster_list.at(i);

        clust.orientation = globalpos.theta;  //TODO: Calcular velocidade e orientacao corretas (provavelmente usando um tracker)
        clust.linear_velocity = 0.0;
        clust.track_id = cluster_id;
        clust.last_detection_timestamp = image_msg->timestamp;
        clust.cluster_type = tp;

        clusters.push_back(clust);
    }

    build_moving_objects_message(clusters);

    publish_moving_objects_message(image_msg->timestamp);

    fps = 1.0 / (carmen_get_time() - start_time);
    start_time = carmen_get_time();

#ifdef SHOW_DETECTIONS
    //show_detections(rgb_image, laser_points_in_camera_box_list, predictions, bouding_boxes_list,
    	//	hood_removal_percentage, fps, rddf_points);
#endif
}


void
velodyne_partial_scan_message_handler(carmen_velodyne_partial_scan_message *velodyne_message)
{
    velodyne_message_arrange = velodyne_message;

    carmen_velodyne_camera_calibration_arrange_velodyne_vertical_angles_to_true_position(velodyne_message_arrange);

    carmen_velodyne_partial_scan_message velodyne_copy;

    velodyne_copy.host = velodyne_message_arrange->host;
    velodyne_copy.number_of_32_laser_shots = velodyne_message_arrange->number_of_32_laser_shots;

    velodyne_copy.partial_scan = (carmen_velodyne_32_laser_shot *) malloc(
            sizeof(carmen_velodyne_32_laser_shot) * velodyne_message_arrange->number_of_32_laser_shots);

    memcpy(velodyne_copy.partial_scan, velodyne_message_arrange->partial_scan,
           sizeof(carmen_velodyne_32_laser_shot) * velodyne_message_arrange->number_of_32_laser_shots);

    velodyne_copy.timestamp = velodyne_message_arrange->timestamp;

    velodyne_vector.push_back(velodyne_copy);

    if (velodyne_vector.size() > maxPositions)
    {
        free(velodyne_vector.begin()->partial_scan);
        velodyne_vector.erase(velodyne_vector.begin());
    }
}


void
localize_ackerman_globalpos_message_handler(carmen_localize_ackerman_globalpos_message *globalpos_message)
{
    globalpos.theta = globalpos_message->globalpos.theta;
    globalpos.x = globalpos_message->globalpos.x;
    globalpos.y = globalpos_message->globalpos.y;
}


void
shutdown_module(int signo)
{
    if (signo == SIGINT) {
        carmen_ipc_disconnect();
        cvDestroyAllWindows();

        printf("Neural Object Detector 2: disconnected.\n");
        exit(0);
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////


void
subscribe_messages()
{
    carmen_bumblebee_basic_subscribe_stereoimage(camera, NULL, (carmen_handler_t) image_handler, CARMEN_SUBSCRIBE_LATEST);

    carmen_velodyne_subscribe_partial_scan_message(NULL, (carmen_handler_t) velodyne_partial_scan_message_handler, CARMEN_SUBSCRIBE_LATEST);

    carmen_localize_ackerman_subscribe_globalpos_message(NULL, (carmen_handler_t) localize_ackerman_globalpos_message_handler, CARMEN_SUBSCRIBE_LATEST);

    //carmen_behavior_selector_subscribe_goal_list_message(NULL, (carmen_handler_t) behaviour_selector_goal_list_message_handler, CARMEN_SUBSCRIBE_LATEST);



    carmen_rddf_subscribe_annotation_message(NULL, (carmen_handler_t) rddf_annotation_message_handler, CARMEN_SUBSCRIBE_LATEST);

    carmen_subscribe_message((char *) CARMEN_BEHAVIOR_SELECTOR_ROAD_PROFILE_MESSAGE_NAME, (char *) CARMEN_BEHAVIOR_SELECTOR_ROAD_PROFILE_MESSAGE_FMT,
        			NULL, sizeof (carmen_behavior_selector_road_profile_message), (carmen_handler_t) rddf_handler, CARMEN_SUBSCRIBE_LATEST);
}


int
read_parameters(int argc, char **argv)
{
    camera = atoi(argv[1]);             // Define the camera to be used
    camera_side = atoi(argv[2]);        // 0 For left image 1 for right image

    int num_items;

    char bumblebee_string[256];
    char camera_string[256];

    sprintf(bumblebee_string, "%s%d", "bumblebee_basic", camera); // Geather the cameri ID
    sprintf(camera_string, "%s%d", "camera", camera);

    carmen_param_t param_list[] =
    {
		{bumblebee_string, (char*) "fx", CARMEN_PARAM_DOUBLE, &camera_parameters.fx_factor, 0, NULL },
		{bumblebee_string, (char*) "fy", CARMEN_PARAM_DOUBLE, &camera_parameters.fy_factor, 0, NULL },
		{bumblebee_string, (char*) "cu", CARMEN_PARAM_DOUBLE, &camera_parameters.cu_factor, 0, NULL },
		{bumblebee_string, (char*) "cv", CARMEN_PARAM_DOUBLE, &camera_parameters.cv_factor, 0, NULL },
		{bumblebee_string, (char*) "pixel_size", CARMEN_PARAM_DOUBLE, &camera_parameters.pixel_size, 0, NULL },

		{"sensor_board_1", (char*) "x",     CARMEN_PARAM_DOUBLE, &board_pose.position.x, 0, NULL },
		{"sensor_board_1", (char*) "y",     CARMEN_PARAM_DOUBLE, &board_pose.position.y, 0, NULL },
		{"sensor_board_1", (char*) "z",     CARMEN_PARAM_DOUBLE, &board_pose.position.z, 0, NULL },
		{"sensor_board_1", (char*) "roll",  CARMEN_PARAM_DOUBLE, &board_pose.orientation.roll, 0, NULL },
		{"sensor_board_1", (char*) "pitch", CARMEN_PARAM_DOUBLE, &board_pose.orientation.pitch, 0, NULL },
		{"sensor_board_1", (char*) "yaw",   CARMEN_PARAM_DOUBLE, &board_pose.orientation.yaw, 0, NULL },

		{camera_string, (char*) "x",     CARMEN_PARAM_DOUBLE, &camera_pose.position.x, 0, NULL },
		{camera_string, (char*) "y",     CARMEN_PARAM_DOUBLE, &camera_pose.position.y, 0, NULL },
		{camera_string, (char*) "z",     CARMEN_PARAM_DOUBLE, &camera_pose.position.z, 0, NULL },
		{camera_string, (char*) "roll",  CARMEN_PARAM_DOUBLE, &camera_pose.orientation.roll, 0, NULL },
		{camera_string, (char*) "pitch", CARMEN_PARAM_DOUBLE, &camera_pose.orientation.pitch, 0, NULL },
		{camera_string, (char*) "yaw",   CARMEN_PARAM_DOUBLE, &camera_pose.orientation.yaw, 0, NULL }
    };


    num_items = sizeof(param_list) / sizeof(param_list[0]);
    carmen_param_install_params(argc, argv, param_list, num_items);

    return 0;
}


int
main(int argc, char **argv)
{
    if ((argc != 3))
        carmen_die("%s: Wrong number of parameters. neural_object_detector requires 2 parameter and received %d. \n Usage: %s <camera_number> <camera_side(0-left; 1-right)\n>",
                   argv[0], argc - 1, argv[0]);

    int device_id = 0;

    string darknet_home = getenv("DARKNET_HOME");  // Get environment variable pointing path of darknet
    if (darknet_home.empty())
        printf("Cannot find darknet path. Check if you have correctly set DARKNET_HOME environment variable.\n");

    string cfg_filename = darknet_home + "/cfg/neural_object_detector_yolo.cfg";
    string weight_filename = darknet_home + "/yolo.weights";
    string class_names_file = darknet_home + "/data/coco.names";
    obj_names = objects_names_from_file(class_names_file);

    darknet = new Detector(cfg_filename, weight_filename, device_id);
    carmen_test_alloc(darknet);

//#ifdef SHOW_DETECTIONS
//    cv::namedWindow("Neural Object Detector", cv::WINDOW_AUTOSIZE);
//#endif

    setlocale(LC_ALL, "C");

    carmen_ipc_initialize(argc, argv);

    signal(SIGINT, shutdown_module);

    read_parameters(argc, argv);

    subscribe_messages();

    //printf("%lf %lf\n", rddf_msg.x, rddf_msg.y);

    carmen_ipc_dispatch();

    return 0;
}
