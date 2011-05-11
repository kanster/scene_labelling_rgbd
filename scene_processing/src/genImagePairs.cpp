/* 
 * File:   genImagePairs.cpp
 * Author: aa755
 *
 * Created on May 10, 2011, 10:12 PM
 */

#include <iostream>
#include <boost/thread.hpp>

#include <stdint.h>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>



#include "pcl/io/pcd_io.h"
#include "pcl/point_types.h"
#include <pcl/point_types.h>

#include <pcl_ros/io/bag_io.h>

#include "pcl_visualization/pcl_visualizer.h"
#include <dynamic_reconfigure/server.h>
#include <scene_processing/pcmergerConfig.h>
#include "transformation.h"
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "color.cpp"


typedef pcl::PointXYZRGBCamSL PointT;
//#include "CombineUtils.h"
//#include "CombineUtils.h"

void saveFloatImage ( const char* filename, const IplImage * image )
{
  IplImage * saveImage = cvCreateImage ( cvGetSize ( image ),
                                             IPL_DEPTH_32F, 3 );
  cvConvertScale ( image, saveImage, 255, 0 );
  cvSaveImage( filename, saveImage);
  cvReleaseImage ( &saveImage );
}

/*
 * 
 */
int
main (int argc, char** argv)
{
  std::string fn;



  //sensor_msgs::PointCloud2 cloud_blob_new;
  //sensor_msgs::PointCloud2 cloud_blob_prev;
  sensor_msgs::PointCloud2 cloud_blob_merged;
  sensor_msgs::PointCloud2ConstPtr cloud_blob_new, cloud_blob_prev;
  //sensor_msgs::Image image;
 
 

  pcl::PointCloud<PointT> cloud_new;
  pcl::PCDWriter writer;



  pcl::PointCloud<PointT>::Ptr cloud_prev_ptr (new pcl::PointCloud<PointT > ());
  //pcl::PointCloud<PointT>::Ptr cloud_new_ptr (new pcl::PointCloud<PointT > ());
  pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_new_ptr (new pcl::PointCloud<pcl::PointXYZRGB > ());
  pcl::PointCloud<PointT>::Ptr cloud_mod_ptr (new pcl::PointCloud<PointT > ());
  pcl::PointCloud<PointT>::Ptr cloud_merged_ptr (new pcl::PointCloud<PointT > ());
  pcl::PointCloud<PointT>::Ptr cloud_merged_backup_ptr (new pcl::PointCloud<PointT > ());
  bool doUpdate = false;
  bool Merged = false;
  bool ITpresent = false;
  pcl_ros::BAGReader reader;
  int skipNum = 20;

  scene_processing::pcmergerConfig InitialTransformConfig;
  char *topic = "/camera/rgb/points";
  if (argc > 2)
    topic = argv[2];

  if (!reader.open (argv[1], "/rgbdslam/my_clouds"))
    {
      cout << "Couldn't read bag file on topic" << (topic);
      return (-1);
    }
  cloud_blob_new = reader.getNextCloud ();
  pcl::fromROSMsg (*cloud_blob_new, *cloud_new_ptr);

  if (pcl::io::loadPCDFile (argv[2], cloud_blob_merged) == -1)
    {
      ROS_ERROR ("Couldn't read file test_pcd.pcd");
      return (-1);
    }
  //    ROS_INFO("Loaded %d data points from test_pcd.pcd with the following fields: %s", (int) (cloud_blob.width * cloud_blob.height), pcl::getFieldsList(cloud_blob).c_str());

  pcl::fromROSMsg (cloud_blob_merged, *cloud_merged_ptr);

  
  for (int i = 0; i < cloud_merged_ptr->size (); i++)
    {
      if (cloud_merged_ptr->points[i].cameraIndex == 0)
        {
          PointT src;
          pcl::PointXYZRGB tmp;
          src = cloud_merged_ptr->points[i];
          int count = 0;
          for (int j = 0; j < cloud_new_ptr->size (); j++)
            {
              tmp = cloud_new_ptr->points[j];
              if (src.x == tmp.x && src.y == tmp.y && src.z == tmp.z && src.rgb == tmp.rgb)
                {
                  //cloud_new_ptr->points[j].label = src.label;
                  count++;
                }
            }
          //cout<<count<<endl;
          assert (count == 1);
        }   
    }

  CvSize size;
  size.height=480;
  size.width=640;
  IplImage * image = cvCreateImage ( size, IPL_DEPTH_32F, 3 );
  
          pcl::PointXYZRGB tmp;
  for(int x=0;x<size.width;x++)
    for(int y=0;y<size.height;y++)
      {
        int index=x+y*size.width;
        tmp= cloud_new_ptr->points[index];
        ColorRGB tmpColor(tmp.rgb);
        CV_IMAGE_ELEM ( image, float, y, 3 * x ) = tmpColor.b;
        CV_IMAGE_ELEM ( image, float, y, 3 * x + 1 ) = tmpColor.g;
        CV_IMAGE_ELEM ( image, float, y, 3 * x + 2 ) = tmpColor.r;
      }

saveFloatImage ( "snapshot.png", image );
  return 0;
}
