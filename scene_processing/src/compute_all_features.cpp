#include "float.h"
#include "math.h"
#include <iostream>
#include <fstream>
#include "pcl/io/pcd_io.h"
#include "pcl/point_types.h"
#include "pcl/filters/passthrough.h"
#include "pcl/filters/extract_indices.h"
#include "pcl/features/intensity_spin.h"
#include "pcl/features/normal_3d.h"
#include "descriptors_3d/all_descriptors.h"
#include <point_cloud_mapping/kdtree/kdtree_ann.h>
#include <vector>
#include "sensor_msgs/point_cloud_conversion.h"
#include "color.cpp"
#include "pcl/kdtree/kdtree.h"
#include "pcl/kdtree/tree_types.h"
#include <point_cloud_mapping/geometry/nearest.h>
//#include <Eig>
//typedef pcl::PointXYGRGBCam PointT;
typedef pcl::PointXYZRGBCamSL PointT;

typedef  pcl::KdTree<PointT> KdTree;
typedef  pcl::KdTree<PointT>::Ptr KdTreePtr;


using namespace pcl;

class BinningInfo
{
  float max;
  float min;
  int numBins;
  float binSize;
public:
  
  
  BinningInfo(float min_,float max_,int numBins_)
  {
    max=max_;
    min=min_;
    numBins=numBins_;
    assert(max>min);
    binSize=(max-min)/numBins;
    
  }

  int
  getBinIndex (float value)
  {
    assert(value>=min);
    assert(value<=max);
    
    int bin =  ((value -min) / binSize);

    assert (bin <= numBins);

    if (bin == numBins)
      {
        bin = numBins - 1;
      }
    
    return bin;

  }

  float
  GetBinSize () const
  {
    return binSize;
  }

  int
  GetNumBins () const
  {
    return numBins;
  }

  float
  GetMin () const
  {
    return min;
  }

  float
  GetMax () const
  {
    return max;
  }
};

void apply_segment_filter(pcl::PointCloud<PointT> &incloud, pcl::PointCloud<PointT> &outcloud, int segment) {
    //ROS_INFO("applying filter");

    outcloud.points.erase(outcloud.points.begin(), outcloud.points.end());

    outcloud.header.frame_id = incloud.header.frame_id;
//    outcloud.points = incloud.points;
    outcloud.points.resize ( incloud.points.size() );

    int j = -1;
    for (size_t i = 0; i < incloud.points.size(); ++i) {

        if (incloud.points[i].segment == segment) {
          j++;
          outcloud.points[j].x = incloud.points[i].x;
          outcloud.points[j].y = incloud.points[i].y;
          outcloud.points[j].z = incloud.points[i].z;
          outcloud.points[j].rgb = incloud.points[i].rgb;
          outcloud.points[j].segment = incloud.points[i].segment;
          outcloud.points[j].label = incloud.points[i].label;

            //     std::cerr<<segment_cloud.points[j].label<<",";
        }
    }
    
   // cout<<j << ","<<segment<<endl;
    if(j>=0)
        outcloud.points.resize ( j+1 );
    else
       outcloud.points.clear ();
}



float getSmallestDistance (const pcl::PointCloud<PointT> &cloud1,const pcl::PointCloud<PointT> &cloud2)
{
  float min_distance = FLT_MAX;
  pcl::PointCloud<PointT>::Ptr small_cloud;
  pcl::PointCloud<PointT>::Ptr big_cloud;
  if (cloud1.points.size() > cloud2.points.size()){
    pcl::PointCloud<PointT>::Ptr cloud_ptr1(new pcl::PointCloud<PointT > (cloud1));
    pcl::PointCloud<PointT>::Ptr cloud_ptr2(new pcl::PointCloud<PointT > (cloud2));
    small_cloud = cloud_ptr2;
    big_cloud = cloud_ptr1;
  }else {
    pcl::PointCloud<PointT>::Ptr cloud_ptr1(new pcl::PointCloud<PointT > (cloud1));
    pcl::PointCloud<PointT>::Ptr cloud_ptr2(new pcl::PointCloud<PointT > (cloud2));
    small_cloud = cloud_ptr1;
    big_cloud = cloud_ptr2;
  }

  pcl::KdTreeFLANN<PointT>::Ptr tree (new pcl::KdTreeFLANN<PointT>);//= boost::make_shared<pcl::KdTreeFLANN<PointT> > ();
  //initTree (0, tree);
  tree->setInputCloud (big_cloud );// ,indicesp);
  std::vector<int> nn_indices;
  nn_indices.resize(2);
  std::vector<float> nn_distances;
  nn_distances.resize(2);
  float tolerance = 0.3;
  
  for (size_t i = 0; i < small_cloud->points.size (); ++i)
  {
  
	//if (!tree->radiusSearch ((*small_cloud).points[i], tolerance, nn_indices, nn_distances))
	tree->nearestKSearch (small_cloud->points[i], 2 , nn_indices, nn_distances);
  	
	for (size_t j = 0; j < nn_indices.size (); ++j)             // nn_indices[0] should be sq_idx
      	{
			
      		//float distance = pow(cloud1.points[i].x - cloud2.points[nn_indices[j]].x,2) + pow(cloud1.points[i].y - cloud2.points[ nn_indices[j]].y,2) + pow(cloud1.points[i].z - cloud2.points[ nn_indices[j]].z,2);
     		 //cout<< "i,j = " << i << "," << j<< " dist = " <<distance << endl;
                 //float a = nn_distances[j];
                 //cout<< nn_distances[j] << " "<< a << endl;
      		 if (min_distance > nn_distances[j])
                 {
                     min_distance = nn_distances[j];
                  //   cout << "changing min_distance to "  << min_distance<< endl;

                 }
	}
  }

/*
  for (size_t i = 0; i < cloud1.points.size (); ++i)
  { 
   for (size_t j = 0; j < cloud2.points.size (); ++j)
   {
      float distance = pow(cloud1.points[i].x - cloud2.points[j].x,2) + pow(cloud1.points[i].y - cloud2.points[j].y,2) + pow(cloud1.points[i].z - cloud2.points[j].z,2);
     // cerr<< "i,j = " << i << "," << j<< " dist = " <<distance << endl;
      if (min_distance > distance) min_distance = distance;
   }
  }
*/
  return sqrt(min_distance) ;
}


void get_neighbors ( const std::vector<pcl::PointCloud<PointT> > &segment_clouds, map< pair <int,int> , float > &distance_matrix, map <int , vector <int> > &neighbor_map )
{
   float tolerance =0.3;
// get distance matrix
    for (size_t i = 0; i< segment_clouds.size(); i++)
    {
      for (size_t j = i+1; j < segment_clouds.size() ; j++)
      { 
         float dist = getSmallestDistance(segment_clouds[i],segment_clouds[j]);
         distance_matrix[make_pair(segment_clouds[i].points[1].segment,segment_clouds[j].points[1].segment)] = dist;
         distance_matrix[make_pair(segment_clouds[j].points[1].segment,segment_clouds[i].points[1].segment)] = dist;
      }
 //     std::cerr<< "size of segment " << i << " : " << segment_clouds[i].points.size() << "\t and label is: " << segment_clouds[i].points[1].label <<"\n";
    }
// get neighbour map
    for ( map< pair <int,int> , float >::iterator it=distance_matrix.begin() ; it != distance_matrix.end(); it++ )
    {   
      if((*it).second < tolerance)  neighbor_map[(*it).first.first].push_back((*it).first.second);
 //     cout << (*it).first.first << "," << (*it).first.second <<" => " << (*it).second << endl;
    }
/*
// printing the neighbor_map
    for ( map< int, vector <int> >::iterator it=neighbor_map.begin() ; it != neighbor_map.end(); it++ )
    {
      cout << (*it).first << " => ";
      for (vector<int>::iterator it2 = (*it).second.begin(); it2 != (*it).second.end(); it2++)
        cout << "," << (*it2) ;
      cout << endl;
    }*/
}


void getMinMax(const pcl::PointCloud<PointT> &cloud, const pcl::PointIndices &indices, Eigen::Vector4f &min_p, Eigen::Vector4f &max_p) {
    min_p.setConstant(FLT_MAX);
    max_p.setConstant(-FLT_MAX);
    min_p[3] = max_p[3] = 0;

    for (size_t i = 0; i < indices.indices.size(); ++i) {
        if (cloud.points[indices.indices[i]].x < min_p[0]) min_p[0] = cloud.points[indices.indices[i]].x;
        if (cloud.points[indices.indices[i]].y < min_p[1]) min_p[1] = cloud.points[indices.indices[i]].y;
        if (cloud.points[indices.indices[i]].z < min_p[2]) min_p[2] = cloud.points[indices.indices[i]].z;

        if (cloud.points[indices.indices[i]].x > max_p[0]) max_p[0] = cloud.points[indices.indices[i]].x;
        if (cloud.points[indices.indices[i]].y > max_p[1]) max_p[1] = cloud.points[indices.indices[i]].y;
        if (cloud.points[indices.indices[i]].z > max_p[2]) max_p[2] = cloud.points[indices.indices[i]].z;
    }
}

void getMinMax(const pcl::PointCloud<PointT> &cloud, Eigen::Vector4f &min_p, Eigen::Vector4f &max_p) {
    min_p.setConstant(FLT_MAX);
    max_p.setConstant(-FLT_MAX);
    min_p[3] = max_p[3] = 0;

    for (size_t i = 0; i < cloud.points.size(); ++i) {
        if (cloud.points[i].x < min_p[0]) min_p[0] = cloud.points[i].x;
        if (cloud.points[i].y < min_p[1]) min_p[1] = cloud.points[i].y;
        if (cloud.points[i].z < min_p[2]) min_p[2] = cloud.points[i].z;

        if (cloud.points[i].x > max_p[0]) max_p[0] = cloud.points[i].x;
        if (cloud.points[i].y > max_p[1]) max_p[1] = cloud.points[i].y;
        if (cloud.points[i].z > max_p[2]) max_p[2] = cloud.points[i].z;
    }
}

void getCentroid(const pcl::PointCloud<PointT> &cloud, Eigen::Vector4f &centroid)
{
    centroid[0]=0;
    centroid[1]=0;
    centroid[2]=0;
    for (size_t i = 0; i < cloud.points.size(); ++i) {
			//assert(cloud.points[i].z>=0);
        centroid[0] += cloud.points[i].x;
        centroid[1] += cloud.points[i].y;
        centroid[2] += cloud.points[i].z;
			//assert(centroid[2]>=0);
    }
    centroid[0] = centroid[0]/(cloud.points.size()-1) ;
    centroid[1] = centroid[1]/(cloud.points.size()-1) ;
    centroid[2] = centroid[2]/(cloud.points.size()-1) ;
}

void getNormal(const pcl::PointCloud<PointT> &cloud, Eigen::Vector3d &normal)
{
   Eigen::Matrix3d eigen_vectors;
   Eigen::Vector3d eigen_values;
   sensor_msgs::PointCloud2 cloudMsg2;
   pcl::toROSMsg (cloud,cloudMsg2);
   sensor_msgs::PointCloud cloudMsg;
   sensor_msgs::convertPointCloud2ToPointCloud(cloudMsg2,cloudMsg);
  cloud_geometry::nearest::computePatchEigen (cloudMsg,eigen_vectors,eigen_values);
  
  double minEigV=DBL_MAX;
  
  for(int i=0;i<3;i++)
    {
      
          cout<<"eig value:"<<eigen_values(i)<<endl;
      if(minEigV>eigen_values(i))
        {
          minEigV=eigen_values(i);
          cout<<"min eig value:"<<minEigV<<endl;
          normal=eigen_vectors.col(i);
        }
    }  
}

void get_feature_average(vector<vector<float> > &descriptor_results, vector<float> &avg_feats) {

    vector<vector<float> >::iterator it = descriptor_results.begin();
    while (it->size() == 0) it++;
    avg_feats.resize(it->size());

    int count = 0;
    for (vector<vector<float> >::iterator it = descriptor_results.begin(); it < descriptor_results.end(); it++) {
        if (it->size() > 0) {
            count++;
        }
        vector<float>::iterator i = avg_feats.begin();
        for (vector<float>::iterator it2 = it->begin(); it2 < it->end(); it2++, i++) {
            *i = *i + *it2;
        }

    }
    // std::cerr << "average features" << std::endl;
    int c = 0;
    for (vector<float>::iterator i = avg_feats.begin(); i < avg_feats.end(); i++) {
        c++;
        *i = *i / count;
        //     std::cerr << c << " : " << *i << ",\t";
    }
    std::cerr << std::endl;
}

void get_feature_histogram(vector<vector<float> > &descriptor_results, vector< vector<float> > &result, vector<BinningInfo> binningInfos) {

    // num_bin = 5;
    vector<vector<float> >::iterator it = descriptor_results.begin();
    int count=it->size();
    result.resize(count);
    // set size of result vector
    
    vector<BinningInfo>::iterator binningInfo = binningInfos.begin();
    for (vector<vector<float> >::iterator ires = result.begin(); ires < result.end(); ires++,binningInfo++)
      {
        ires->resize(binningInfo->GetNumBins ());       
      }
/*
    while (it->size() == 0) it++;
    max.resize(it->size(), -FLT_MAX);
    min.resize(it->size(), FLT_MAX);
    int count = 0;
    // find the bin size by finding the max and min of feature value
    for (vector<vector<float> >::iterator it = descriptor_results.begin(); it < descriptor_results.end(); it++) {
        vector<float>::iterator imax = max.begin();
        vector<float>::iterator imin = min.begin();
        int c = 0;
        for (vector<float>::iterator it2 = it->begin(); it2 < it->end(); it2++, imax++, imin++) {
            c++;
            //   std::cerr << c << " : " << *it2 << ",\t";
            if (*imax < *it2) {
                *imax = *it2;
            }
            if (*imin > *it2) {
                *imin = *it2;
            }
        }
        //  std::cerr << std::endl;
    }

*/
    // fill the histogram
  
    for (vector<vector<float> >::iterator it_point = descriptor_results.begin(); it_point < descriptor_results.end(); it_point++) { // iterate over points
        vector<BinningInfo>::iterator binningInfo = binningInfos.begin();
        
        vector<vector<float> >::iterator ires = result.begin();

        assert(count==it_point->size ());//missing features NOT allowed for now.
        
        for (vector<float>::iterator it_feature = it_point->begin(); it_feature < it_point->end(); it_feature++, binningInfo++, ires++) { // iterate over features of the point

            int bin = binningInfo->getBinIndex (*it_feature);
            
            //   ROS_INFO("%f %d %d",bin_size,bin,(*ires).size());
            
            (*ires)[bin] += 1;
        }
    }

    // normalize and print histogram
 //   std::cerr << "historam \n";
    int c1 = 0, c2 = 0;
    for (vector< vector<float> >::iterator i = result.begin(); i < result.end(); i++) {
        c1++;
   //     std::cerr << "histogram for feature:" << c1 << "\n";
        for (vector<float>::iterator i2 = i->begin(); i2 < i->end(); i2++) {
            c2++;
            *i2 = *i2 / count;
          //  std::cerr << c2 << " : " << *i2 << ",\t";
        }
  //      std::cerr << std::endl;
    }
  //  std::cerr << std::endl;

}


// concat feats (vector-wise) to features vector

void concat_feats(vector<float> &features, vector<vector<float> > &feats) {
    for (vector<vector<float> >::iterator it = feats.begin(); it < feats.end(); it++) {

        //       vector<float>::iterator i = features.end();
        for (vector<float>::iterator it2 = it->begin(); it2 < it->end(); it2++) {
            features.push_back(*it2);
        }
    }
}

// concat feats vector to features vector

void concat_feats(vector<float> &features, vector<float> &feats) {
    for (vector<float>::iterator it = feats.begin(); it < feats.end(); it++) {

        features.push_back(*it);

    }
}

void get_color_features(const pcl::PointCloud<PointT> &cloud, vector<float> &features) {
 int num_bin_H=9;
 int num_bin_S=3;
 int num_bin_V=3;
    // histogram and average of hue and intensity

    vector<vector<float> > hist_features;
    vector<float> avg_features;
    vector < vector <float> > color_features(cloud.points.size());
    vector <vector <float> >::iterator it = color_features.begin();
    for (size_t i = 0; i < cloud.points.size(); ++i, it++) {
        ColorRGB c(cloud.points[i].rgb);
        (*it).push_back(c.H);
        (*it).push_back(c.S);
        (*it).push_back(c.V);
    }
    
    vector<BinningInfo> binnigInfos;
    binnigInfos.push_back (BinningInfo(0,360,num_bin_H));
    binnigInfos.push_back (BinningInfo(0,1,num_bin_S));
    binnigInfos.push_back (BinningInfo(0,1,num_bin_V));
    get_feature_histogram(color_features, hist_features,binnigInfos);
    get_feature_average(color_features, avg_features);

  //  concat_feats(features, hist_features);
    concat_feats(features, avg_features);

}

void get_global_features(const pcl::PointCloud<PointT> &cloud, vector<float> &features) {
    
    Eigen::Vector4f min_p;
    Eigen::Vector4f max_p;
    // get bounding box features
    getMinMax(cloud, min_p, max_p);
    //  ROS_INFO("minp : %f,%f,%f\t maxp : %f,%f,%f", min_p[0], min_p[1], min_p[2], max_p[0], max_p[1], max_p[2]);
    //  ROS_INFO("size of all_descriptor_results : %d", all_descriptor_results[1].size());
    // add bounding box features
    features.push_back(max_p[0] - min_p[0]);
    features.push_back(max_p[1] - min_p[1]);
    features.push_back(max_p[2] - min_p[2]);

    Eigen::Vector4f centroid;
    getCentroid(cloud,centroid);
    features.push_back(centroid[0]);
    features.push_back(centroid[1]);
    features.push_back(centroid[2]);
    //features.push_back(centroid[2]*centroid[2]);
    //Eigen::Vector3d normal;
    //getNormal (cloud, normal);
    //features.push_back (fabs (normal(2)));

}

void get_shape_features(const pcl::PointCloud<PointT> &cloud, vector<float> &features, int num_bin ) {


    sensor_msgs::PointCloud cloud_blob2;
    sensor_msgs::PointCloud2 cloud_tmp;

    SpectralAnalysis sa(0.05);
    SpinImageNormal spin_image(0.025, 0.025, 5, 4, false, sa);
    ShapeSpectral shape_spectral(sa);
    OrientationNormal o_normal(0, 0, 1, sa);
    OrientationTangent o_tangent(0, 0, 1, sa);
    Position position;
    BoundingBoxSpectral bbox_spectral(1.0, sa);

    // histogram feats
    vector<Descriptor3D*> descriptors_3d;
    descriptors_3d.push_back(&shape_spectral);
    descriptors_3d.push_back(&o_normal);
    //descriptors_3d.push_back(&o_tangent);
    //descriptors_3d.push_back(&position);
    //descriptors_3d.push_back(&bbox_spectral);
    
    pcl::toROSMsg(cloud, cloud_tmp);
    sensor_msgs::convertPointCloud2ToPointCloud(cloud_tmp, cloud_blob2);
    cloud_kdtree::KdTreeANN pt_cloud_kdtree(cloud_blob2);
    vector<const geometry_msgs::Point32*> interest_pts;
    if (cloud.points.size() < 2000) {
        interest_pts.resize(cloud_blob2.points.size());
        for (size_t i = 0; i < cloud_blob2.points.size(); i++) {
            interest_pts[i] = &(cloud_blob2.points[i]);
        }
    } else {
        interest_pts.resize(1000);
        map<int, int> in;
        int count = 0;
        while (count < 1000) {
            int a = rand() % cloud_blob2.points.size();
            if (in.find(a) == in.end()) {
                in[a] = 1;
                interest_pts[count] = &(cloud_blob2.points[a]);
            }
            count++;
        }

    }


    //     vector<vector<float> >  descriptor_results;
    //     spin_image.compute(cloud_blob2, pt_cloud_kdtree, interest_pts, descriptor_results);
    unsigned int nbr_descriptors = descriptors_3d.size();
    vector<vector<vector<float> > > all_descriptor_results(nbr_descriptors);

  //  vector<vector<vector<float> > > hist_feats(nbr_descriptors);
    vector<vector<float> > avg_feats1(nbr_descriptors);
    for (unsigned int i = 0; i < nbr_descriptors; i++) {
        descriptors_3d[i]->compute(cloud_blob2, pt_cloud_kdtree, interest_pts, all_descriptor_results[i]);
        
       // get_feature_histogram(all_descriptor_results[i], hist_feats[i], num_bin);
      //  concat_feats(features, hist_feats[i]);
        get_feature_average(all_descriptor_results[i], avg_feats1[i]);
        concat_feats(features, avg_feats1[i]);
    }

/*

    // average feats
    vector<Descriptor3D*> descriptors_3d_avg;
    descriptors_3d_avg.push_back(&spin_image);

    unsigned int nbr_descriptors_avg = descriptors_3d_avg.size();
    vector<vector<vector<float> > > all_avg_descriptor_results(nbr_descriptors);
    vector<vector<float> > avg_feats(nbr_descriptors_avg);
    for (unsigned int i = 0; i < nbr_descriptors_avg; i++) {
        std::cerr << "avg featnum: " << i << "\n";
        descriptors_3d_avg[i]->compute(cloud_blob2, pt_cloud_kdtree, interest_pts, all_avg_descriptor_results[i]);
        get_feature_average(all_avg_descriptor_results[i], avg_feats[i]);
        concat_feats(features, avg_feats[i]);
    }
*/
}

void get_avg_normals(vector<pcl::PointCloud<PointT> > &segment_clouds, vector<pcl::Normal> &normalsOut )
{
 
	pcl::KdTreeFLANN<PointT>::Ptr tree (new pcl::KdTreeFLANN<PointT> ());
	pcl::PointCloud<pcl::Normal>::Ptr cloud_normals (new pcl::PointCloud<pcl::Normal> ());
	pcl::NormalEstimation<PointT, pcl::Normal> ne;
	ne.setSearchMethod (tree);
  	ne.setKSearch (50);
    for (size_t i = 0; i< segment_clouds.size(); i++)
    {
 
    	pcl::Normal avgNormal;
		pcl::PointCloud<PointT>::Ptr cloudptr (new pcl::PointCloud<PointT> (segment_clouds[i]));
  		ne.setInputCloud (cloudptr);
  		ne.compute (*cloud_normals);
		for (size_t i = 0; i < (*cloud_normals).points.size(); ++i)
		{
			avgNormal.normal[0] += (*cloud_normals).points[i].normal[0];
        	avgNormal.normal[1] += (*cloud_normals).points[i].normal[1];
			avgNormal.normal[2] += (*cloud_normals).points[i].normal[2]; 
		}
		avgNormal.normal[0] = avgNormal.normal[0]/(*cloud_normals).points.size();
    	avgNormal.normal[1] = avgNormal.normal[1]/(*cloud_normals).points.size();
		avgNormal.normal[2] = avgNormal.normal[2]/(*cloud_normals).points.size();
		normalsOut.push_back(avgNormal);
	}
}

void get_pair_features( int segment_id, vector<int>  &neighbor_list,
                        map< pair <int,int> , float > &distance_matrix,
						std::map<int,int>  &segment_num_index_map,
						vector<pcl::Normal> &cloud_normals,
                        map <int, vector<float> >&features,
                        map < int, vector<float> > &edge_features) {

    int centroid_x_index = 9;
    int centroid_y_index = 10;
    int centroid_z_index = 11;
    int normal_angle_index = 3;

    for (vector<int>::iterator it = neighbor_list.begin(); it != neighbor_list.end(); it++) {

        int seg2_id = *it;
        // horizontal distance between centroids:
        float centroid_dist_horz = sqrt(pow(features[segment_id][centroid_x_index] - features[seg2_id][centroid_x_index], 2)
                + pow(features[segment_id][centroid_y_index] - features[seg2_id][centroid_y_index], 2));
        edge_features[seg2_id].push_back(centroid_dist_horz);
        // difference in z coordinates of the centroids
        edge_features[seg2_id].push_back((features[segment_id][centroid_z_index ] - features[seg2_id][centroid_z_index ]));
        //cerr << "edge feature for edge (" << seg1_id << "," << seg2_id << ")  = " << centroid_z_diff << endl;
        
        // distance between closest points
        edge_features[seg2_id].push_back(distance_matrix[make_pair(segment_id,seg2_id)]);

        // difference of angles with vertical
        edge_features[seg2_id].push_back((acos(features[segment_id][normal_angle_index]) - acos(features[seg2_id][normal_angle_index])));
		
		// dot product of normals
		edge_features[seg2_id].push_back( cloud_normals[segment_num_index_map[segment_id]].normal[0]*cloud_normals[segment_num_index_map[seg2_id]].normal[0] + cloud_normals[segment_num_index_map[segment_id]].normal[1]*cloud_normals[segment_num_index_map[seg2_id]].normal[1]  + cloud_normals[segment_num_index_map[segment_id]].normal[2]*cloud_normals[segment_num_index_map[seg2_id]].normal[2] );

        float innerness = sqrt(pow(features[segment_id][centroid_x_index],2) + pow(features[segment_id][centroid_y_index], 2))
                - sqrt(pow(features[seg2_id][centroid_x_index],2) + pow(features[seg2_id][centroid_y_index], 2));
        
        edge_features[seg2_id].push_back(innerness);
    }

}


int main(int argc, char** argv) {

    int scene_num = atoi(argv[2]);
    sensor_msgs::PointCloud2 cloud_blob;
    pcl::PointCloud<PointT> cloud;
    std::ofstream labelfile, nfeatfile, efeatfile;

    //labelfile.open("data_labels.txt",ios::app);
    nfeatfile.open("data_nodefeats.txt",ios::app);
    efeatfile.open("data_edgefeats.txt",ios::app);

    // read the pcd file

    if (pcl::io::loadPCDFile(argv[1], cloud_blob) == -1) {
        ROS_ERROR("Couldn't read file test_pcd.pcd");
        return (-1);
    }
    ROS_INFO("Loaded %d data points from test_pcd.pcd with the following fields: %s", (int) (cloud_blob.width * cloud_blob.height), pcl::getFieldsList(cloud_blob).c_str());

    // convert to templated message type

    pcl::fromROSMsg(cloud_blob, cloud);

    pcl::PointCloud<PointT>::Ptr cloud_ptr(new pcl::PointCloud<PointT > (cloud));

    pcl::PointCloud<PointT>::Ptr cloud_filtered(new pcl::PointCloud<PointT > ());
    pcl::PointCloud<PointT>::Ptr cloud_seg(new pcl::PointCloud<PointT > ());
    //pcl::PointCloud<PointXYZI>::Ptr cloud_seg (new pcl::PointCloud<PointXYZI> ());
    std::vector<pcl::PointCloud<PointT> > segment_clouds;
    std::map<int,int>  segment_num_index_map;
    pcl::PointIndices::Ptr segment_indices(new pcl::PointIndices());

    // get segments

    // find the max segment number
    int max_segment_num = 0;
    for (size_t i = 0; i < cloud.points.size(); ++i) {
        if (max_segment_num < cloud.points[i].segment) {
            max_segment_num = (int) cloud.points[i].segment;
        }
    }

    ExtractIndices<PointT> extract;

    int index_ = 0;
    for (int seg = 1; seg <= max_segment_num; seg++) {
        //vector<float> features;
        //int label;
        //segment_indices->indices.clear();
        //int sizec = 0;
        //for (size_t i = 0; i < cloud.points.size(); ++i) {
			//assert(cloud.points[i].z>=0);
          //  if (cloud.points[i].segment == seg) {
            //    sizec++;
              //  segment_indices->indices.push_back(i);
               // label = cloud.points[i].label;
           // }
       // }
       // if(label!=0) cout << "segment: "<< seg << " label: " << label  << " size: " << sizec << endl;
        //extract.setInputCloud(cloud_ptr);
        //extract.setIndices(segment_indices);
        //extract.setNegative(false);
        //extract.filter(*cloud_seg);
        apply_segment_filter (*cloud_ptr,*cloud_seg,seg);
        
        //if (label!=0) cout << "segment: "<< seg << " label: " << label << " size: " << cloud_seg->points.size() << endl;
        if (!cloud_seg->points.empty () && cloud_seg->points.size() > 10  && cloud_seg->points[1].label != 0) {
         //std::cout << seg << ". Cloud size after extracting : " << cloud_seg->points.size() << std::endl;
			segment_clouds.push_back(*cloud_seg);
			segment_num_index_map[cloud_seg->points[1].segment] = index_;
			index_ ++; 
        }
    }
    map< pair <int,int> , float > distance_matrix;
    map <int , vector <int> > neighbor_map;
    get_neighbors ( segment_clouds, distance_matrix, neighbor_map );


    // for each segment compute node featuers
    int num_bin_shape = 3;
    map < int , vector<float> > features;
    for (size_t i = 0; i< segment_clouds.size(); i++)
    {
     //   vector<float> features;
        int seg_id = segment_clouds[i].points[1].segment;
        // get color features
        //cout << "computing color features" << std::endl;
        get_color_features(segment_clouds[i], features[seg_id]);

        // get shape features
        get_shape_features(segment_clouds[i], features[seg_id], num_bin_shape);

        // get bounding box and centroid point features
        get_global_features(segment_clouds[i], features[seg_id]);

    }
    vector<pcl::Normal> cloud_normals;
    get_avg_normals(segment_clouds,cloud_normals);
    // print the node features
    for (map< int, vector<float> >::iterator it = features.begin(); it != features.end(); it++ ){
        
        cerr << (*it).first << ":\t";
        nfeatfile <<  scene_num << "\t" << (*it).first << "\t" << segment_clouds[segment_num_index_map[(*it).first]].points[1].label << "\t";
        for (vector<float>::iterator it2 = (*it).second.begin(); it2 != (*it).second.end(); it2++) {
           cerr << *it2 << "\t";
           nfeatfile <<  *it2 << "\t";
        }
        cerr << endl;
        nfeatfile << "\n";
    }
    cerr << "\n";
    map < int, vector <float> > edge_features;
    // find pairwise features
    for ( map< int, vector<int> >::iterator it=neighbor_map.begin() ; it != neighbor_map.end(); it++) {

        edge_features.clear();
        get_pair_features((*it).first, (*it).second, distance_matrix, segment_num_index_map , cloud_normals, features, edge_features);
        // print pair-wise features
        for (map< int, vector<float> >::iterator it2 = edge_features.begin(); it2 != edge_features.end(); it2++) {
            cerr << "edge: ("<< (*it).first << "," << (*it2).first << "):\t";
            efeatfile << scene_num << "\t" << (*it).first << "\t" << (*it2).first << "\t" << segment_clouds[segment_num_index_map[(*it).first]].points[1].label << "\t" << segment_clouds[segment_num_index_map[(*it2).first]].points[1].label ;
            for (vector<float>::iterator it3 = (*it2).second.begin(); it3 != (*it2).second.end(); it3++) {
                cerr << *it3 << "\t";
                efeatfile << "\t" <<*it3 ;
            }
            cerr << endl;
            efeatfile << endl;
        }

    }
  

    cout << "DONE!!\n";
    // write features to file
/*    for (vector<float>::iterator it = features.begin(); it < features.end(); it++) {
      outfile << *it << "\t";
    }
    outfile << "\n";
 */
    //labelfile.close();
    nfeatfile.close();
    efeatfile.close();



}


