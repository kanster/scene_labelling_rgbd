//Documentation see header file
#include "pcl/ros/conversions.h"
#include <pcl/io/io.h>
#include "pcl/common/transform.h"
#include "pcl_ros/transforms.h"
#include "openni_listener.h"
#include <cv_bridge/CvBridge.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <cv.h>
#include <ctime>
#include <sensor_msgs/PointCloud2.h>


OpenNIListener::OpenNIListener( ros::NodeHandle nh,  const char* visual_topic, 
                               const char* depth_topic, const char* info_topic, 
                               const char* cloud_topic, const char* filename , unsigned int step ) 
: visual_sub_ (nh, visual_topic, 10),
  depth_sub_(nh, depth_topic, 10),
  info_sub_(nh, info_topic, 10),
  cloud_sub_(nh, cloud_topic, 10),
  sync_(MySyncPolicy(10),  visual_sub_, depth_sub_, info_sub_, cloud_sub_),
  callback_counter_(0),
  step_(step)
{
  // ApproximateTime takes a queue size as its constructor argument, hence MySyncPolicy(10)
  sync_.registerCallback(boost::bind(&OpenNIListener::cameraCallback, this, _1, _2, _3, _4));
  ROS_INFO_STREAM("OpenNIListener listening to " << visual_topic << ", " << depth_topic \
                   << ", " << info_topic << " and " << cloud_topic << "\n"); 

  bag_.open(filename, rosbag::bagmode::Write);

//  matcher_ = new cv::BruteForceMatcher<cv::L2<float> >() ;
/*  pub_cloud_ = nh.advertise<sensor_msgs::PointCloud2> (
            "/rgbdslam/input/points", 10);
  pub_info_ = nh.advertise<sensor_msgs::CameraInfo> (
            "/rgbdslam/input/camera_info", 10);
  pub_visual_ = nh.advertise<sensor_msgs::Image> (
           "/rgbdslam/input/image_mono", 10);
  pub_depth_ = nh.advertise<sensor_msgs::Image> (
           "/rgbdslam/input/depth_image", 10);
*/

}

void OpenNIListener::cameraCallback (const sensor_msgs::ImageConstPtr& visual_img_msg, 
                                     const sensor_msgs::ImageConstPtr& depth_img_msg,   
                                     const sensor_msgs::CameraInfoConstPtr& cam_info,
                                     const sensor_msgs::PointCloud2ConstPtr& point_cloud) {

   ROS_INFO("Received data from kinect");
  
   if(++callback_counter_%step_ == 0) {
     bag_.write ( "/camera/rgb/image_mono" , visual_img_msg->header.stamp, visual_img_msg );
     bag_.write ( "/camera/depth/image" , depth_img_msg->header.stamp, depth_img_msg );
     bag_.write ( "/camera/rgb/camera_info" , cam_info->header.stamp, cam_info );
     bag_.write ( "/camera/rgb/points" , point_cloud->header.stamp, point_cloud );
   }
/*  if(++callback_counter_ % 3 != 0 || pause_) return;
  //Get images into correct format
	sensor_msgs::CvBridge bridge;
	cv::Mat depth_float_img = bridge.imgMsgToCv(depth_img_msg); 
	cv::Mat visual_img =  bridge.imgMsgToCv(visual_img_msg, "mono8");
  if(visual_img.rows != depth_float_img.rows || visual_img.cols != depth_float_img.cols){
    ROS_ERROR("Depth and Visual image differ in size!");
    return;
  }
   
*/

}

using namespace cv;

void transformPointCloud (const Eigen::Matrix4f &transform, const sensor_msgs::PointCloud2 &in,
                          sensor_msgs::PointCloud2 &out)
{
  // Get X-Y-Z indices
  int x_idx = pcl::getFieldIndex (in, "x");
  int y_idx = pcl::getFieldIndex (in, "y");
  int z_idx = pcl::getFieldIndex (in, "z");

  if (x_idx == -1 || y_idx == -1 || z_idx == -1) {
    ROS_ERROR ("Input dataset has no X-Y-Z coordinates! Cannot convert to Eigen format.");
    return;
  }

  if (in.fields[x_idx].datatype != sensor_msgs::PointField::FLOAT32 || 
      in.fields[y_idx].datatype != sensor_msgs::PointField::FLOAT32 || 
      in.fields[z_idx].datatype != sensor_msgs::PointField::FLOAT32) {
    ROS_ERROR ("X-Y-Z coordinates not floats. Currently only floats are supported.");
    return;
  }
  // Check if distance is available
  int dist_idx = pcl::getFieldIndex (in, "distance");
  // Copy the other data
  if (&in != &out)
  {
    out.header = in.header;
    out.height = in.height;
    out.width  = in.width;
    out.fields = in.fields;
    out.is_bigendian = in.is_bigendian;
    out.point_step   = in.point_step;
    out.row_step     = in.row_step;
    out.is_dense     = in.is_dense;
    out.data.resize (in.data.size ());
    // Copy everything as it's faster than copying individual elements
    memcpy (&out.data[0], &in.data[0], in.data.size ());
  }

  Eigen::Array4i xyz_offset (in.fields[x_idx].offset, in.fields[y_idx].offset, in.fields[z_idx].offset, 0);

  for (size_t i = 0; i < in.width * in.height; ++i) {
    Eigen::Vector4f pt (*(float*)&in.data[xyz_offset[0]], *(float*)&in.data[xyz_offset[1]], *(float*)&in.data[xyz_offset[2]], 1);
    Eigen::Vector4f pt_out;
    
    bool max_range_point = false;
    int distance_ptr_offset = i*in.point_step + in.fields[dist_idx].offset;
    float* distance_ptr = (dist_idx < 0 ? NULL : (float*)(&in.data[distance_ptr_offset]));
    if (!std::isfinite (pt[0]) || !std::isfinite (pt[1]) || !std::isfinite (pt[2])) {
      if (distance_ptr==NULL || !std::isfinite(*distance_ptr)) { // Invalid point 
        pt_out = pt;
      } else { // max range point
        pt[0] = *distance_ptr;  // Replace x with the x value saved in distance
        pt_out = transform * pt;
        max_range_point = true;
        //std::cout << pt[0]<<","<<pt[1]<<","<<pt[2]<<" => "<<pt_out[0]<<","<<pt_out[1]<<","<<pt_out[2]<<"\n";
      }
    } else {
      pt_out = transform * pt;
    }

    if (max_range_point) {
      // Save x value in distance again
      *(float*)(&out.data[distance_ptr_offset]) = pt_out[0];
      pt_out[0] = std::numeric_limits<float>::quiet_NaN();
      //std::cout << __PRETTY_FUNCTION__<<": "<<i << "is a max range point.\n";
    }

    memcpy (&out.data[xyz_offset[0]], &pt_out[0], sizeof (float));
    memcpy (&out.data[xyz_offset[1]], &pt_out[1], sizeof (float));
    memcpy (&out.data[xyz_offset[2]], &pt_out[2], sizeof (float));
    
    xyz_offset += in.point_step;
  }

  // Check if the viewpoint information is present
  int vp_idx = pcl::getFieldIndex (in, "vp_x");
  if (vp_idx != -1) {
    // Transform the viewpoint info too
    for (size_t i = 0; i < out.width * out.height; ++i) {
      float *pstep = (float*)&out.data[i * out.point_step + out.fields[vp_idx].offset];
      // Assume vp_x, vp_y, vp_z are consecutive
      Eigen::Vector4f vp_in (pstep[0], pstep[1], pstep[2], 1);
      Eigen::Vector4f vp_out = transform * vp_in;

      pstep[0] = vp_out[0];
      pstep[1] = vp_out[1];
      pstep[2] = vp_out[2];
    }
  }
}

void depthToCV8UC1(const cv::Mat& float_img, cv::Mat& mono8_img){
  //Process images
  if(mono8_img.rows != float_img.rows || mono8_img.cols != float_img.cols){
    mono8_img = cv::Mat(float_img.size(), CV_8UC1);}
  //The following doesn't work due to NaNs
  //double minVal, maxVal; 
  //minMaxLoc(float_img, &minVal, &maxVal);
  //ROS_DEBUG("Minimum/Maximum Depth in current image: %f/%f", minVal, maxVal);
  //mono8_img = cv::Scalar(0);
  //cv::line( mono8_img, cv::Point2i(10,10),cv::Point2i(200,100), cv::Scalar(255), 3, 8);
  cv::convertScaleAbs(float_img, mono8_img, 100, 0.0);
}

//Little debugging helper functions
std::string openCVCode2String(unsigned int code){
  switch(code){
    case 0 : return std::string("CV_8UC1" );
    case 8 : return std::string("CV_8UC2" );
    case 16: return std::string("CV_8UC3" );
    case 24: return std::string("CV_8UC4" );
    case 2 : return std::string("CV_16UC1");
    case 10: return std::string("CV_16UC2");
    case 18: return std::string("CV_16UC3");
    case 26: return std::string("CV_16UC4");
    case 5 : return std::string("CV_32FC1");
    case 13: return std::string("CV_32FC2");
    case 21: return std::string("CV_32FC3");
    case 29: return std::string("CV_32FC4");
  }
  return std::string("Unknown");
}

void printMatrixInfo(cv::Mat& image){
  ROS_DEBUG_STREAM("Matrix Type:" << openCVCode2String(image.type()) <<  " rows: " <<  image.rows  <<  " cols: " <<  image.cols);
}
